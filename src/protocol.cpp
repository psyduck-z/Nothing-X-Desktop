#include "protocol.h"

#include <QRegularExpression>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <QtEndian>
#include <cstring>

namespace Proto {

// CRC-16/MODBUS: reflected poly 0xA001, init 0xFFFF, no final xor.
quint16 crc16(const QByteArray &bytes) {
    quint16 crc = 0xFFFF;
    for (char c : bytes) {
        crc ^= quint8(c);
        for (int i = 0; i < 8; ++i)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc;
}

static void appendLe16(QByteArray &out, quint16 v) {
    out.append(char(v & 0xFF));
    out.append(char(v >> 8));
}

static quint16 le16(const QByteArray &b, int at) {
    return quint16(quint8(b[at])) | quint16(quint8(b[at + 1])) << 8;
}

QByteArray encode(quint16 cmd, const QByteArray &payload, quint8 fsn) {
    QByteArray out;
    out.reserve(10 + payload.size());
    out.append(char(0x55));
    appendLe16(out, DefaultCtrl);
    appendLe16(out, cmd);
    appendLe16(out, quint16(payload.size()));
    out.append(char(fsn));
    out.append(payload);
    appendLe16(out, crc16(out));
    return out;
}

QByteArray fromHex(const char *hex) { return QByteArray::fromHex(QByteArray(hex)); }

void Decoder::feed(const QByteArray &bytes) { m_buffer.append(bytes); }

std::optional<Frame> Decoder::next() {
    constexpr int Header = 8;
    constexpr int MaxPayload = 1024;
    for (;;) {
        const int start = m_buffer.indexOf(char(0x55));
        if (start < 0) {
            m_buffer.clear();
            return std::nullopt;
        }
        if (start > 0)
            m_buffer.remove(0, start);
        if (m_buffer.size() < Header)
            return std::nullopt;

        const quint16 ctrl = le16(m_buffer, 1);
        const quint16 len = le16(m_buffer, 5);
        const bool hasCrc = ctrl & 0x20;
        // The high byte of ctrl is the device type (1 earbuds; 3 on the
        // CRC-less Fast Pair style pushes). Anything else means this 0x55 is
        // not a frame start, and trusting its length would stall the stream.
        const quint8 deviceType = ctrl >> 8;
        if ((deviceType != 0x01 && deviceType != 0x03) || len > MaxPayload) {
            m_buffer.remove(0, 1);
            continue;
        }
        const int total = Header + len + (hasCrc ? 2 : 0);
        if (m_buffer.size() < total)
            return std::nullopt;
        if (hasCrc && crc16(m_buffer.left(Header + len)) != le16(m_buffer, Header + len)) {
            // Not a real frame start (or a corrupt one): resync after this byte.
            m_buffer.remove(0, 1);
            continue;
        }
        Frame frame;
        frame.ctrl = ctrl;
        frame.cmd = le16(m_buffer, 3);
        frame.fsn = quint8(m_buffer[7]);
        frame.payload = m_buffer.mid(Header, len);
        m_buffer.remove(0, total);
        return frame;
    }
}

// --- Values -----------------------------------------------------------------

static quint8 at(const QByteArray &b, int i) { return i < b.size() ? quint8(b[i]) : 0; }

static float floatLe(const QByteArray &b, int at) {
    if (at + 4 > b.size())
        return 0;
    const quint32 raw = qFromLittleEndian<quint32>(b.constData() + at);
    float f;
    std::memcpy(&f, &raw, 4);
    return f;
}

static void appendFloatLe(QByteArray &out, double value) {
    const float f = float(value);
    quint32 raw;
    std::memcpy(&raw, &f, 4);
    char bytes[4];
    qToLittleEndian(raw, bytes);
    out.append(bytes, 4);
}

Battery parseBattery(const QByteArray &payload) {
    // count, then (part id, level) pairs; bit 7 of the level means charging.
    // The case is only reported while its lid is open.
    Battery b;
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const int o = 1 + i * 2;
        if (o + 2 > payload.size())
            break;
        const quint8 raw = at(payload, o + 1);
        const BatteryLevel level{raw & 0x7F, bool(raw & 0x80)};
        switch (at(payload, o)) {
        case Left: b.left = level; break;
        case Right: b.right = level; break;
        case Case: b.caseUnit = level; break;
        case Single: b.single = level; break;
        default: break;
        }
    }
    return b;
}

std::optional<Anc> parseAnc(const QByteArray &payload) {
    if (payload.size() < 2)
        return std::nullopt;
    return Anc{at(payload, 1)};
}

QByteArray ancPayload(quint8 mode) { return QByteArray::fromRawData("\x01", 1) + char(mode) + char(0); }

QList<Gesture> parseGestures(const QByteArray &payload) {
    QList<Gesture> out;
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const int o = 1 + i * 4;
        if (o + 4 > payload.size())
            break;
        out.append({at(payload, o), at(payload, o + 1), at(payload, o + 2), at(payload, o + 3)});
    }
    return out;
}

QByteArray gesturePayload(const Gesture &g) {
    QByteArray out;
    out.append(char(1));
    out.append(char(g.device));
    out.append(char(g.button));
    out.append(char(g.type));
    out.append(char(g.action));
    return out;
}

QByteArray customEqPayload(const CustomEq &eq, const CustomEqShape &s) {
    // Band order on the wire is peak (mid), high shelf (treble), low shelf
    // (bass); the total gain pulls the whole curve down by the largest boost
    // so it can never clip.
    const double total = -std::max({eq.bass, eq.mid, eq.treble, 0.0});
    QByteArray out;
    out.append(char(3));
    appendFloatLe(out, total);
    auto band = [&](quint8 type, double gain, double freq, double q) {
        out.append(char(type));
        appendFloatLe(out, gain);
        appendFloatLe(out, freq);
        appendFloatLe(out, q);
    };
    band(1, eq.mid, s.peakFreq, s.peakQ);
    band(2, eq.treble, s.highFreq, s.highQ);
    band(0, eq.bass, s.lowFreq, s.lowQ);
    out.append(QByteArray(9, 0));
    return out;
}

std::optional<CustomEq> parseCustomEq(const QByteArray &payload) {
    const int n = at(payload, 0);
    if (n < 3 || payload.size() < 5 + 3 * 13)
        return std::nullopt;
    CustomEq eq;
    for (int i = 0; i < 3; ++i) {
        const int o = 5 + i * 13;
        const quint8 type = at(payload, o);
        const double gain = std::round(floatLe(payload, o + 1) * 10.0) / 10.0;
        if (type == 0)
            eq.bass = gain;
        else if (type == 1)
            eq.mid = gain;
        else if (type == 2)
            eq.treble = gain;
    }
    return eq;
}

QByteArray advancedEqPayload(const QVector<EqBand> &bands) {
    double maxGain = 0;
    for (const auto &b : bands)
        maxGain = std::max(maxGain, b.gain);
    QByteArray out;
    out.append(char(0)); // profile index
    out.append(char(bands.size()));
    appendFloatLe(out, -maxGain);
    for (const auto &b : bands) {
        out.append(char(b.type));
        appendFloatLe(out, b.gain);
        appendFloatLe(out, b.frequency);
        appendFloatLe(out, b.q);
    }
    return out;
}

QVector<EqBand> parseAdvancedEq(const QByteArray &payload) {
    QVector<EqBand> bands;
    const int n = at(payload, 1);
    for (int i = 0; i < n; ++i) {
        const int o = 6 + i * 13;
        if (o + 13 > payload.size())
            break;
        EqBand b;
        b.type = at(payload, o);
        b.gain = std::round(floatLe(payload, o + 1) * 10.0) / 10.0;
        b.frequency = std::round(floatLe(payload, o + 5));
        b.q = std::round(floatLe(payload, o + 9) * 100.0) / 100.0;
        bands.append(b);
    }
    return bands;
}

QVector<EqBand> defaultAdvancedEq() {
    QVector<EqBand> bands;
    for (double f : {55.0, 110.0, 220.0, 440.0, 1320.0, 3300.0, 6600.0, 13200.0})
        bands.append({1, 0.0, f, 1.0});
    return bands;
}

QString macFromBytes(const QByteArray &six) {
    QStringList parts;
    for (char c : six)
        parts << QStringLiteral("%1").arg(quint8(c), 2, 16, QLatin1Char('0')).toUpper();
    return parts.join(':');
}

QByteArray macToBytes(const QString &address) {
    QByteArray out;
    for (const auto &part : address.split(QRegularExpression("[:-]")))
        out.append(char(part.toUInt(nullptr, 16)));
    return out;
}

QList<PairedDevice> parseDualList(const QByteArray &payload, int *reportedCount) {
    QList<PairedDevice> out;
    const int n = at(payload, 2);
    if (reportedCount)
        *reportedCount = n;
    int o = 3;
    for (int i = 0; i < n; ++i) {
        if (o + 8 > payload.size())
            break;
        const quint8 flags = at(payload, o);
        PairedDevice d;
        d.thisHost = flags & 0xF0;
        d.connected = flags & 0x0F;
        d.address = macFromBytes(payload.mid(o + 1, 6));
        const int nameLength = at(payload, o + 7) & 0x7F;
        d.name = QString::fromUtf8(payload.mid(o + 8, nameLength));
        out.append(d);
        o += 8 + nameLength;
    }
    return out;
}

QByteArray dualConnectPayload(const QString &address, bool connect) {
    return QByteArray(1, char(connect ? 1 : 0)) + macToBytes(address);
}

std::optional<PairedDevice> parseDualDevicePush(const QByteArray &payload) {
    if (payload.size() < 7)
        return std::nullopt;
    PairedDevice d;
    d.connected = at(payload, 0) == 1;
    d.address = macFromBytes(payload.mid(1, 6));
    return d;
}

DeviceInfo parseDeviceInfo(const QByteArray &payload) {
    // One leading count byte, then text lines "part,type,value": part 2 left,
    // 3 right, 4 case; type 2 firmware, 4 serial number.
    DeviceInfo info;
    const QString text = QString::fromUtf8(payload.mid(1));
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
        const QStringList f = line.split(',');
        if (f.size() < 3 || f[2].trimmed().isEmpty())
            continue;
        const int part = f[0].toInt(), type = f[1].toInt();
        const QString value = f.mid(2).join(',').trimmed();
        if (type == 4 && info.serial.isEmpty())
            info.serial = value;
        if (type == 2) {
            if (part == 2) info.leftFirmware = value;
            else if (part == 3) info.rightFirmware = value;
            else if (part == 4) info.caseFirmware = value;
        }
    }
    return info;
}

std::optional<bool> parseInEar(const QByteArray &payload) {
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const int o = 1 + i * 2;
        if (o + 2 > payload.size())
            break;
        if (at(payload, o) == 1)
            return at(payload, o + 1) != 0;
    }
    return std::nullopt;
}

WearState parseWearState(const QByteArray &payload) {
    WearState w;
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const int o = 1 + i * 2;
        if (o + 2 > payload.size())
            break;
        if (at(payload, o) == Left)
            w.left = at(payload, o + 1);
        else if (at(payload, o) == Right)
            w.right = at(payload, o + 1);
    }
    return w;
}

std::optional<FitResult> parseFitResult(const QByteArray &payload) {
    if (payload.size() < 2)
        return std::nullopt;
    return FitResult{at(payload, 0), at(payload, 1)};
}

QString parseModelId(const QByteArray &payload) {
    if (payload.size() < 2)
        return {};
    // Little-endian 0xB173 for the Ear (3).
    return QString::number(le16(payload, 0), 16).toUpper();
}

QByteArray timePayload(qint64 unixSeconds) {
    char bytes[4];
    qToBigEndian(quint32(unixSeconds), bytes);
    return QByteArray(bytes, 4);
}

} // namespace Proto
