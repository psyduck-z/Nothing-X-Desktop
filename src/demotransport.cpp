#include "demotransport.h"

#include <QTimer>

using namespace Proto;

DemoTransport::DemoTransport(QObject *parent) : Transport(parent) {
    // Factory state of a black Ear (3), in the official payload layouts.
    m_state[Cmd::FirmwareRead] = "1.0.1.62";
    m_state[Cmd::CaseFirmwareRead] = QByteArray::fromHex("010000120000000a");
    m_state[Cmd::InfoRead] = QByteArray(1, 5) +
                             "2,1,1.0\n2,2,1.0.1.62\n3,2,1.0.1.62\n4,2,1.0.0.18\n2,4,SH10DEM0000173\n";
    // What a real Ear (3) reports (its B201 id, listed under B173).
    m_state[Cmd::ModelRead] = QByteArray::fromHex("01b2");
    m_state[Cmd::ColorRead] = QByteArray(1, 1);
    // Both buds in the ears and connected; the case is open.
    m_state[Cmd::StatusRead] = QByteArray::fromHex("03028403840401");
    m_state[Cmd::AncRead] = QByteArray::fromHex("010400020400");
    m_state[Cmd::EqModeRead] = le16Bytes(0);
    m_state[Cmd::CustomEqRead] = customEqPayload({2, 0, 1}, {});
    m_state[Cmd::AdvancedEqModeRead] = QByteArray(1, 0);
    m_state[Cmd::AdvancedEqRead] = advancedEqPayload(defaultAdvancedBands(), -6.0f);
    m_state[Cmd::BassRead] = bassPayload(true, 3);
    m_state[Cmd::SpatialRead] = spatialPayload(0);
    m_state[Cmd::MutualExclusionRead] = QByteArray(1, 1);
    m_state[Cmd::LdacRead] = le16Bytes(0);
    m_state[Cmd::LowLagRead] = le16Bytes(2);
    m_state[Cmd::InEarRead] = QByteArray::fromHex("010101");
    m_state[Cmd::GesturesRead] =
        QByteArray::fromHex("0902010209030102090201030803010308020107160301071602010901030109010401070B");
    m_state[Cmd::PersonalSoundRead] = QByteArray(1, 0);
    m_state[Cmd::DualRead] = QByteArray(1, 1);
    m_state[Cmd::SuperMicRead] = QByteArray(1, 1);
    m_state[Cmd::CallTransparencyRead] = QByteArray(1, 0);
    m_state[Cmd::WalkieTalkieRead] = QByteArray(1, 0);
    m_hosts = {
        {QByteArray::fromHex("a0b1c2d3e4f5"), "Desktop", true, true},
        {QByteArray::fromHex("1c2b3a495867"), "Phone", true, false},
    };

    // The buds slowly drain while playing; the case charges.
    m_drain.setInterval(45 * 1000);
    connect(&m_drain, &QTimer::timeout, this, [this] {
        m_left = qMax(5, m_left - 1);
        m_right = qMax(5, m_right - 1);
        m_case = qMin(100, m_case + 1);
        push(Cmd::BatteryEvent, batteryPayload());
    });
}

QByteArray DemoTransport::batteryPayload() const {
    QByteArray b(1, 3);
    b.append(char(PartLeft)).append(char(m_left));
    b.append(char(PartRight)).append(char(m_right));
    b.append(char(PartCase)).append(char(m_case | (m_caseCharging ? 0x80 : 0)));
    return b;
}

QByteArray DemoTransport::dualListPayload() const {
    QByteArray b;
    b.append(char(1)).append(char(0)).append(char(m_hosts.size()));
    for (const Host &h : m_hosts) {
        b.append(char((h.thisHost ? 0x10 : 0) | (h.connected ? 1 : 0)));
        b.append(h.mac);
        b.append(char(h.name.size()));
        b.append(h.name);
    }
    return b;
}

void DemoTransport::open(const QString &) {
    m_open = true;
    QTimer::singleShot(m_latency * 4, this, [this] {
        if (!m_open)
            return;
        emit opened();
        // Like the real earbuds, ask the host for the time once connected.
        QTimer::singleShot(m_latency * 10, this, [this] { push(Cmd::TimeRequest, {}); });
    });
    m_drain.start();
}

void DemoTransport::close() {
    if (!m_open)
        return;
    m_open = false;
    m_drain.stop();
    QTimer::singleShot(0, this, &Transport::closed);
}

void DemoTransport::write(const QByteArray &bytes) {
    m_decoder.feed(bytes);
    while (auto f = m_decoder.next()) {
        m_sent.append(*f);
        const Frame frame = *f;
        if (m_latency == 0)
            handle(frame);
        else
            QTimer::singleShot(m_latency, this, [this, frame] { handle(frame); });
    }
}

void DemoTransport::send(quint16 cmd, int status, quint8 fsn, const QByteArray &payload) {
    if (!m_open)
        return;
    // Replies and notifications come back with bit 15 cleared and the status
    // in the low bits of the control word.
    QByteArray frame = encode(cmd & 0x7FFF, payload, fsn);
    const quint16 control = HostControl | quint16(status & 0x1F);
    frame[1] = char(control & 0xFF);
    frame[2] = char(control >> 8);
    const quint16 crc = crc16(frame.left(frame.size() - 2));
    frame[frame.size() - 2] = char(crc & 0xFF);
    frame[frame.size() - 1] = char(crc >> 8);
    emit received(frame);
}

void DemoTransport::push(quint16 cmd, const QByteArray &payload) { send(cmd, 0, 0, payload); }

void DemoTransport::handle(const Frame &f) {
    const QByteArray &p = f.payload;
    if (f.isRead()) {
        QByteArray reply;
        if (f.cmd == Cmd::BatteryRead)
            reply = batteryPayload();
        else if (f.cmd == Cmd::DualListRead)
            reply = dualListPayload();
        else
            reply = m_state.value(f.cmd);
        send(f.cmd, m_state.contains(f.cmd) || !reply.isEmpty() ? Ok : NotSupported, f.fsn, reply);
        return;
    }
    if (!f.isWrite())
        return;
    if (m_rejectStatus) {
        send(f.cmd, m_rejectStatus, f.fsn, {});
        m_rejectStatus = 0;
        return;
    }
    auto u8 = [&p](int i) { return i < p.size() ? quint8(p[i]) : 0; };
    switch (f.cmd) {
    case Cmd::AncWrite: {
        AncState s = parseAnc(m_state[Cmd::AncRead]);
        const int mode = u8(1);
        QByteArray anc = ancPayload(mode);
        const int level = mode >= AncHigh && mode <= AncAdaptive ? mode : s.lastLevel;
        anc.append(char(2)).append(char(level)).append(char(0));
        m_state[Cmd::AncRead] = anc;
        send(f.cmd, Ok, f.fsn, {});
        push(Cmd::AncEvent, anc);
        return;
    }
    case Cmd::EqModeWrite: m_state[Cmd::EqModeRead] = p; break;
    case Cmd::CustomEqWrite: m_state[Cmd::CustomEqRead] = p; break;
    case Cmd::AdvancedEqModeWrite: m_state[Cmd::AdvancedEqModeRead] = QByteArray(1, char(u8(0))); break;
    case Cmd::AdvancedEqWrite: m_state[Cmd::AdvancedEqRead] = p; break;
    case Cmd::BassWrite: m_state[Cmd::BassRead] = p; break;
    case Cmd::SpatialWrite: m_state[Cmd::SpatialRead] = p; break;
    case Cmd::LdacWrite: m_state[Cmd::LdacRead] = p; break;
    case Cmd::LowLagWrite: m_state[Cmd::LowLagRead] = p; break;
    case Cmd::InEarWrite: m_state[Cmd::InEarRead] = p; break;
    case Cmd::PersonalSoundWrite: m_state[Cmd::PersonalSoundRead] = p; break;
    case Cmd::SuperMicWrite: m_state[Cmd::SuperMicRead] = p; break;
    case Cmd::CallTransparencyWrite: m_state[Cmd::CallTransparencyRead] = p; break;
    case Cmd::WalkieTalkieWrite:
        m_state[Cmd::WalkieTalkieRead] = p;
        if (u8(0) == 0)
            m_state[Cmd::SuperMicRead] = QByteArray(1, 0);
        break;
    case Cmd::GesturesWrite: {
        QList<Gesture> all = parseGestures(m_state[Cmd::GesturesRead]);
        for (const Gesture &n : parseGestures(p)) {
            bool found = false;
            for (Gesture &g : all)
                if (g.device == n.device && g.gesture == n.gesture) {
                    g.action = n.action;
                    found = true;
                }
            if (!found)
                all.append(n);
        }
        m_state[Cmd::GesturesRead] = gesturesPayload(all);
        break;
    }
    case Cmd::DualWrite:
        m_state[Cmd::DualRead] = p;
        send(f.cmd, Ok, f.fsn, {});
        push(Cmd::DualSwitchEvent, p);
        return;
    case Cmd::DualConnectWrite: {
        const QByteArray mac = p.mid(1, 6);
        for (Host &h : m_hosts)
            if (h.mac == mac && !h.thisHost)
                h.connected = u8(0) == 1;
        send(f.cmd, Ok, f.fsn, {});
        QTimer::singleShot(qMax(1, m_latency * 8), this, [this] { push(Cmd::DualDeviceEvent, {}); });
        return;
    }
    case Cmd::FitTestWrite:
        send(f.cmd, Ok, f.fsn, {});
        // Left seals well, right needs adjusting: shows both results.
        QTimer::singleShot(m_fitDuration, this, [this] { push(Cmd::FitTestEvent, QByteArray::fromHex("0001")); });
        return;
    default:
        break; // find, time, Essential Space reply: acknowledged
    }
    send(f.cmd, Ok, f.fsn, {});
}
