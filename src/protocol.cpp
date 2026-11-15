#include "protocol.h"

#include <QRegularExpression>
#include <QStringList>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Proto {

// CRC-16/MODBUS: reflected polynomial 0xA001, init 0xFFFF, no final xor
// (the official app's Utils.obtainCrc16).
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

static quint16 le16(const QByteArray &b, qsizetype at) {
    return quint16(quint8(b[at])) | quint16(quint8(b[at + 1])) << 8;
}

QByteArray encode(quint16 cmd, const QByteArray &payload, quint8 fsn) {
    QByteArray out;
    out.reserve(10 + payload.size());
    out.append(char(0x55));
    appendLe16(out, HostControl);
    appendLe16(out, cmd);
    appendLe16(out, quint16(payload.size()));
    out.append(char(fsn));
    out.append(payload);
    appendLe16(out, crc16(out));
    return out;
}

// A plausible frame header at `at`: the start byte, a device type the
// earbuds use (1, or 2/3 on some pushes) and nothing in the unused top bits.
static bool headerAt(const QByteArray &b, qsizetype at) {
    if (b.size() < at + 8 || quint8(b[at]) != 0x55)
        return b.size() >= at + 1 && quint8(b[at]) == 0x55 && b.size() < at + 8;
    const quint16 control = le16(b, at + 1);
    const int deviceType = (control >> 8) & 0x0F;
    return deviceType >= 1 && deviceType <= 3 && (control >> 12) == 0 && le16(b, at + 5) <= 4096;
}

std::optional<Frame> Decoder::next(bool flush) {
    constexpr int Header = 8;
    for (;;) {
        const qsizetype start = m_buffer.indexOf(char(0x55));
        if (start < 0) {
            m_buffer.clear();
            return std::nullopt;
        }
        if (start > 0)
            m_buffer.remove(0, start);
        if (m_buffer.size() < Header)
            return std::nullopt;
        if (!headerAt(m_buffer, 0)) {
            // Not a frame start (a stray 0x55): resynchronise after it.
            m_buffer.remove(0, 1);
            continue;
        }
        const quint16 control = le16(m_buffer, 1);
        const qsizetype end = Header + le16(m_buffer, 5);
        if (m_buffer.size() < end)
            return std::nullopt;

        qsizetype total = end;
        if (control & CrcBit) {
            const qsizetype extra = m_buffer.size() - end;
            if (extra >= 2) {
                const bool crcOk = crc16(m_buffer.left(end)) == le16(m_buffer, end);
                // Without a matching CRC and with the next frame starting
                // right here, the firmware sent no CRC; otherwise take the two
                // bytes as the CRC (the official app never checks it).
                total = (!crcOk && headerAt(m_buffer, end)) ? end : end + 2;
            } else if (extra == 1 && quint8(m_buffer[end]) == 0x55) {
                total = end;
            } else if (!flush) {
                return std::nullopt;
            }
        }

        Frame frame;
        frame.control = control;
        frame.cmd = le16(m_buffer, 3) | 0x8000;
        frame.fsn = quint8(m_buffer[7]);
        frame.payload = m_buffer.mid(Header, end - Header);
        m_buffer.remove(0, qMin(total, m_buffer.size()));
        return frame;
    }
}

// --- Values -----------------------------------------------------------------

static quint8 at(const QByteArray &b, qsizetype i) { return i >= 0 && i < b.size() ? quint8(b[i]) : 0; }

QByteArray le16Bytes(int value) {
    QByteArray out;
    appendLe16(out, quint16(value));
    return out;
}

int leInt(const QByteArray &payload) {
    int value = 0;
    for (qsizetype i = qMin<qsizetype>(payload.size(), 4) - 1; i >= 0; --i)
        value = (value << 8) | at(payload, i);
    return value;
}

float floatLe(const QByteArray &bytes, qsizetype offset) {
    if (offset < 0 || offset + 4 > bytes.size())
        return 0;
    const quint32 raw = qFromLittleEndian<quint32>(bytes.constData() + offset);
    float f;
    std::memcpy(&f, &raw, 4);
    return f;
}

void appendFloatLe(QByteArray &out, float value) {
    quint32 raw;
    std::memcpy(&raw, &value, 4);
    char bytes[4];
    qToLittleEndian(raw, bytes);
    out.append(bytes, 4);
}

AncState parseAnc(const QByteArray &payload) {
    AncState s;
    for (qsizetype o = 0; o + 3 <= payload.size(); o += 3) {
        if (at(payload, o) == 1)
            s.mode = at(payload, o + 1);
        else if (at(payload, o) == 2)
            s.lastLevel = at(payload, o + 1);
    }
    return s;
}

QByteArray ancPayload(int mode) {
    QByteArray out;
    out.append(char(1));
    out.append(char(mode));
    out.append(char(0));
    return out;
}

static double round1(double v) { return std::round(v * 10.0) / 10.0; }

QByteArray customEqPayload(const CustomEq &eq, const CustomEqShape &s) {
    // Minus the largest boost, so the curve never clips; -0.0 when nothing
    // is boosted, exactly as the official app computes it.
    const float total = -float(std::max({0.0, eq.mid, eq.treble, eq.bass}));
    QByteArray out;
    out.append(char(3));
    appendFloatLe(out, total);
    auto band = [&](quint8 type, double gain, double freq, double q) {
        out.append(char(type));
        appendFloatLe(out, float(gain));
        appendFloatLe(out, float(freq));
        appendFloatLe(out, float(q));
    };
    band(Peak, eq.mid, s.peakFreq, s.peakQ);
    band(HighShelf, eq.treble, s.highFreq, s.highQ);
    band(LowShelf, eq.bass, s.lowFreq, s.lowQ);
    // The app allocates 16 bytes per band but fills 13: 9 zero bytes follow.
    out.append(QByteArray(9, 0));
    return out;
}

std::optional<CustomEq> parseCustomEq(const QByteArray &payload) {
    const int n = at(payload, 0);
    if (n < 1 || payload.size() < 5 + 13)
        return std::nullopt;
    CustomEq eq;
    for (int i = 0; i < n && 5 + i * 13 + 13 <= payload.size(); ++i) {
        const qsizetype o = 5 + i * 13;
        const double gain = round1(floatLe(payload, o + 1));
        switch (at(payload, o)) {
        case LowShelf: eq.bass = gain; break;
        case Peak: eq.mid = gain; break;
        case HighShelf: eq.treble = gain; break;
        default: break;
        }
    }
    return eq;
}

QVector<EqBand> defaultAdvancedBands() {
    QVector<EqBand> bands;
    for (double f : {55.0, 110.0, 220.0, 440.0, 1320.0, 3300.0, 6600.0, 13200.0})
        bands.append({Peak, 0.0, f, 1.0});
    return bands;
}

QByteArray advancedEqPayload(const QVector<EqBand> &bands, float totalGain) {
    QByteArray out;
    out.append(char(0)); // profile index: the app always sends 0
    out.append(char(bands.size()));
    appendFloatLe(out, totalGain);
    for (const EqBand &b : bands) {
        out.append(char(b.type));
        appendFloatLe(out, float(b.gain));
        appendFloatLe(out, float(b.frequency));
        appendFloatLe(out, float(b.q));
    }
    return out;
}

QVector<EqBand> parseAdvancedEq(const QByteArray &payload) {
    QVector<EqBand> bands;
    const int n = at(payload, 1);
    for (int i = 0; i < n; ++i) {
        const qsizetype o = 6 + i * 13;
        if (o + 13 > payload.size())
            break;
        EqBand b;
        // Advanced bands are peaking filters; the app reads type 0 as 1.
        b.type = at(payload, o) == LowShelf ? int(Peak) : int(at(payload, o));
        b.gain = round1(floatLe(payload, o + 1));
        b.frequency = std::clamp(double(std::round(floatLe(payload, o + 5))), 20.0, 20000.0);
        const double q = std::round(floatLe(payload, o + 9) * 100.0) / 100.0;
        b.q = q <= 0 ? 1.0 : q;
        bands.append(b);
    }
    return bands;
}

Bass parseBass(const QByteArray &payload) {
    Bass b;
    b.on = at(payload, 0) == 1;
    const int raw = payload.size() > 1 ? at(payload, 1) : 2;
    b.level = std::clamp(raw / 2, 1, 5);
    return b;
}

QByteArray bassPayload(bool on, int level) {
    QByteArray out;
    out.append(char(on ? 1 : 0));
    out.append(char(std::clamp(level, 1, 5) * 2));
    return out;
}

int parseSpatial(const QByteArray &payload) {
    if (at(payload, 0) != 1)
        return 0;
    return payload.size() > 1 && at(payload, 1) == 1 ? 2 : 1;
}

QByteArray spatialPayload(int mode) {
    QByteArray out;
    out.append(char(mode > 0 ? 1 : 0));
    out.append(char(mode == 2 ? 1 : 0));
    return out;
}

void mergeBattery(Battery &into, const QByteArray &payload) {
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const qsizetype o = 1 + i * 2;
        if (o + 2 > payload.size())
            break;
        const quint8 raw = at(payload, o + 1);
        const BatteryLevel level{raw & 0x7F, bool(raw & 0x80)};
        switch (at(payload, o)) {
        case PartLeft: into.left = level; break;
        case PartRight: into.right = level; break;
        case PartCase: into.caseUnit = level; break;
        default: break;
        }
    }
}

void mergeWear(Wear &into, const QByteArray &payload) {
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const qsizetype o = 1 + i * 2;
        if (o + 2 > payload.size())
            break;
        const int flags = at(payload, o + 1);
        switch (at(payload, o)) {
        case PartLeft: into.left = flags; break;
        case PartRight: into.right = flags; break;
        case PartCase: into.caseUnit = flags; break;
        default: break;
        }
    }
}

QString parseFirmware(const QByteArray &payload) {
    // Accepted only when it looks like a version: four dotted numbers.
    const QString text = QString::fromLatin1(payload).trimmed();
    static const QRegularExpression version(R"(^\d+(\.\d+){3,}$)");
    return version.match(text).hasMatch() ? text : QString();
}

QString parseCaseFirmware(const QByteArray &payload) {
    if (payload.size() < 4)
        return {};
    QStringList parts;
    for (int i = 0; i < 4; ++i)
        parts << QString::number(qint8(payload[i]));
    return parts.join('.');
}

DeviceInfo parseDeviceInfo(const QByteArray &payload) {
    // A count byte, then text lines "device,type,value": type 2 is the
    // software version and 4 the serial number; device 2/3/4 = L/R/case.
    DeviceInfo info;
    const QString text = QString::fromUtf8(payload.mid(1)).trimmed();
    for (const QString &line : text.split('\n', Qt::SkipEmptyParts)) {
        const QStringList f = line.split(',');
        if (f.size() < 3)
            continue;
        const int part = f[0].trimmed().toInt(), type = f[1].trimmed().toInt();
        const QString value = f.mid(2).join(',').trimmed();
        if (value.isEmpty())
            continue;
        if (type == 4 && info.serial.isEmpty())
            info.serial = value;
        if (type == 2) {
            if (part == PartLeft)
                info.leftFirmware = value;
            else if (part == PartRight)
                info.rightFirmware = value;
            else if (part == PartCase)
                info.caseFirmware = value;
        }
    }
    return info;
}

QString parseModelId(const QByteArray &payload) {
    QString out;
    for (qsizetype i = payload.size() - 1; i >= 0; --i)
        out += QStringLiteral("%1").arg(quint8(payload[i]), 2, 16, QLatin1Char('0')).toUpper();
    return out;
}

QString parseColorId(const QByteArray &payload) {
    return payload.isEmpty() ? QStringLiteral("02") : QStringLiteral("%1").arg(at(payload, 0), 2, 16, QLatin1Char('0')).toUpper();
}

std::optional<bool> parseInEar(const QByteArray &payload) {
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const qsizetype o = 1 + i * 2;
        if (o + 2 > payload.size())
            break;
        if (at(payload, o) == 1)
            return at(payload, o + 1) == 1;
    }
    return std::nullopt;
}

QByteArray inEarPayload(bool on) {
    QByteArray out;
    out.append(char(1));
    out.append(char(1));
    out.append(char(on ? 1 : 0));
    return out;
}

QList<Gesture> parseGestures(const QByteArray &payload) {
    QList<Gesture> out;
    const int n = at(payload, 0);
    for (int i = 0; i < n; ++i) {
        const qsizetype o = 1 + i * 4;
        if (o + 4 > payload.size())
            break;
        out.append({at(payload, o), at(payload, o + 1), at(payload, o + 2), at(payload, o + 3)});
    }
    return out;
}

QByteArray gesturesPayload(const QList<Gesture> &gestures) {
    QByteArray out;
    out.append(char(gestures.size()));
    for (const Gesture &g : gestures) {
        out.append(char(g.device));
        out.append(char(g.button));
        out.append(char(g.gesture));
        out.append(char(g.action));
    }
    return out;
}

QByteArray findPayload(int part, bool ring) {
    QByteArray out;
    if (part != PartWhole)
        out.append(char(part));
    out.append(char(ring ? 1 : 0));
    return out;
}

QString macFromBytes(const QByteArray &six) {
    QStringList parts;
    for (char c : six)
        parts << QStringLiteral("%1").arg(quint8(c), 2, 16, QLatin1Char('0')).toUpper();
    return parts.join(':');
}

QByteArray macToBytes(const QString &address) {
    QByteArray out;
    for (const QString &part : address.split(QRegularExpression("[:-]")))
        out.append(char(part.toUInt(nullptr, 16)));
    return out;
}

DualPage parseDualList(const QByteArray &payload) {
    // [pages][page][count], then per device: [state][mac 6][name length][name].
    // State: low nibble connected, high nibble "this host"; name length bit 7
    // marks a truncated name.
    DualPage page;
    page.pages = at(payload, 0);
    page.page = at(payload, 1);
    const int n = at(payload, 2);
    qsizetype o = 3;
    for (int i = 0; i < n && o + 8 <= payload.size(); ++i) {
        const quint8 state = at(payload, o);
        PairedDevice d;
        d.connected = (state & 0x0F) == 1;
        d.thisHost = ((state & 0xF0) >> 4) == 1;
        d.address = macFromBytes(payload.mid(o + 1, 6));
        const quint8 lengthByte = at(payload, o + 7);
        const bool truncated = lengthByte & 0x80;
        const int length = lengthByte & 0x7F;
        d.name = QString::fromUtf8(payload.mid(o + 8, length));
        if (truncated)
            d.name += QStringLiteral("…");
        page.devices.append(d);
        o += 8 + length;
    }
    return page;
}

QByteArray dualConnectPayload(const QString &address, bool connect) {
    return QByteArray(1, char(connect ? 1 : 0)) + macToBytes(address);
}

QByteArray timePayload(qint64 unixSeconds) {
    char bytes[4];
    qToLittleEndian(quint32(unixSeconds), bytes);
    return QByteArray(bytes, 4);
}

} // namespace Proto
