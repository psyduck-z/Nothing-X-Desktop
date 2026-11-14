#include "demotransport.h"

using namespace Proto;

DemoTransport::DemoTransport(QObject *parent) : Transport(parent) {
    // Replies recorded from a real Ear (3) where available.
    m_state[ModelIdRead] = QByteArray::fromHex("73b1");
    m_state[FirmwareRead] = "1.0.1.69";
    m_state[DeviceInfoRead] = QByteArray::fromHex("09") +
        "2,1,\n2,2,1.0.1.69\n2,4,SH10000000000000\n3,1,\n3,2,1.0.1.69\n3,4,SH10000000000000\n4,1,\n4,2,1.0.1.69\n4,4,SH10000000000000\n";
    m_state[AncRead] = QByteArray::fromHex("010100020100");
    m_state[EqPresetRead] = QByteArray::fromHex("00");
    m_state[CustomEqRead] = customEqPayload({2.0, 0.0, 1.0});
    m_state[AdvancedEqStatusRead] = QByteArray::fromHex("00");
    m_state[AdvancedEqBandsRead] = advancedEqPayload(defaultAdvancedEq());
    m_state[BassRead] = QByteArray::fromHex("0106");
    m_state[SpatialRead] = QByteArray::fromHex("0000");
    m_state[PersonalSoundRead] = QByteArray::fromHex("00");
    m_state[InEarRead] = QByteArray::fromHex("0901010201070109010a010b000e0112011501");
    m_state[WearStateRead] = QByteArray::fromHex("0202050305");
    m_state[LowLatencyRead] = QByteArray::fromHex("02");
    m_state[CodecRead] = QByteArray::fromHex("00");
    m_state[DualEnableRead] = QByteArray::fromHex("01");
    m_state[GesturesRead] = QByteArray::fromHex("0902010209030102090201030803010308020107160301071602010901030109010401070b");
    m_state[SuperMicRead] = QByteArray::fromHex("00");
    m_state[MicModeRead] = QByteArray::fromHex("00");
    m_state[CallTransparencyRead] = QByteArray::fromHex("00");
    m_state[WalkieTalkieRead] = QByteArray::fromHex("00");
    m_state[ProtocolVersionRead] = "4.0";

    // Slowly drain the batteries so the UI's live updates can be seen.
    m_drain.setInterval(45000);
    connect(&m_drain, &QTimer::timeout, this, [this] {
        m_left = qMax(5, m_left - 1);
        m_right = qMax(5, m_right - 1);
        push(BatteryPush, batteryPayload());
    });
}

QByteArray DemoTransport::batteryPayload() const {
    return QByteArray::fromHex("03") + char(Left) + char(m_left) + char(Right) + char(m_right) + char(Case) +
           char(m_case | 0x80);
}

void DemoTransport::open(const QString &) {
    m_open = true;
    m_drain.start();
    QTimer::singleShot(m_latency * 4, this, &Transport::opened);
}

void DemoTransport::close() {
    if (!m_open)
        return;
    m_open = false;
    m_drain.stop();
    emit closed();
}

void DemoTransport::write(const QByteArray &bytes) {
    m_decoder.feed(bytes);
    while (auto f = m_decoder.next())
        handle(*f);
}

void DemoTransport::reply(const Frame &request, const QByteArray &payload) {
    const QByteArray bytes = encode(replyTo(request.cmd), payload, request.fsn);
    if (m_latency == 0)
        emit received(bytes);
    else
        QTimer::singleShot(m_latency, this, [this, bytes] { if (m_open) emit received(bytes); });
}

void DemoTransport::push(quint16 cmd, const QByteArray &payload) {
    const QByteArray bytes = encode(cmd, payload, 0);
    QTimer::singleShot(m_latency, this, [this, bytes] { if (m_open) emit received(bytes); });
}

void DemoTransport::handle(const Frame &f) {
    m_sent.append(f);
    const QByteArray &p = f.payload;
    auto on = [&p](int i) { return QByteArray(1, char(i < p.size() ? p[i] : 0)); };

    switch (f.cmd) {
    case BatteryRead:
        reply(f, batteryPayload());
        return;
    case DualListRead: {
        QByteArray list = QByteArray::fromHex("000002");
        auto add = [&list](quint8 flags, const char *mac, const QByteArray &name) {
            list += char(flags) + QByteArray::fromHex(mac) + char(name.size()) + name;
        };
        add(0x11, "a1b2c3d4e5f6", "Desktop PC");
        add(0x01, "0a1b2c3d4e5f", "Phone");
        reply(f, (p.isEmpty() || p[0] == 0) ? list : QByteArray::fromHex("000000"));
        return;
    }
    case AncWrite:
        m_state[AncRead] = QByteArray::fromHex("01") + on(1) + QByteArray::fromHex("00020100");
        break;
    case EqPresetWrite: m_state[EqPresetRead] = on(0); break;
    case CustomEqWrite: m_state[CustomEqRead] = p; break;
    case AdvancedEqStatusWrite: m_state[AdvancedEqStatusRead] = on(0); break;
    case AdvancedEqBandsWrite: m_state[AdvancedEqBandsRead] = p; break;
    case BassWrite: m_state[BassRead] = p.left(2); break;
    case SpatialWrite: m_state[SpatialRead] = p.left(2); break;
    case PersonalSoundWrite: m_state[PersonalSoundRead] = on(0); break;
    case InEarWrite: {
        QByteArray s = m_state[InEarRead];
        if (p.size() > 2 && s.size() > 2)
            s[2] = p[2];
        m_state[InEarRead] = s;
        break;
    }
    case LowLatencyWrite: m_state[LowLatencyRead] = on(0); break;
    case CodecWrite: m_state[CodecRead] = on(0); break;
    case DualEnableWrite: m_state[DualEnableRead] = on(0); break;
    case SuperMicWrite: m_state[SuperMicRead] = on(0); break;
    case MicModeWrite: m_state[MicModeRead] = on(0); break;
    case CallTransparencyWrite: m_state[CallTransparencyRead] = on(0); break;
    case WalkieTalkieWrite: m_state[WalkieTalkieRead] = on(0); break;
    case GesturesWrite: {
        auto gestures = parseGestures(m_state[GesturesRead]);
        const auto changed = parseGestures(p);
        for (const auto &c : changed)
            for (auto &g : gestures)
                if (g.device == c.device && g.type == c.type)
                    g.action = c.action;
        QByteArray s(1, char(gestures.size()));
        for (const auto &g : gestures)
            s += gesturePayload(g).mid(1);
        m_state[GesturesRead] = s;
        break;
    }
    case FitTestStart:
        QTimer::singleShot(qMax(1, m_latency) * 120, this, [this] { push(FitTestPush, QByteArray::fromHex("0001")); });
        break;
    default:
        break;
    }

    if ((f.cmd & 0xF000) == 0xC000)
        reply(f, m_state.value(f.cmd));
    else
        reply(f, {}); // writes are acknowledged with an empty reply
}
