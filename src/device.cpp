#include "device.h"
#include "settings.h"
#include "transport.h"

#include <QDateTime>

using namespace Proto;

static constexpr int ReplyTimeoutMs = 1200;

Device::Device(Transport *transport, Settings *settings, QObject *parent)
    : QObject(parent), m_transport(transport), m_settings(settings), m_model(Models::generic({})) {
    m_transport->setParent(this);
    m_bands = defaultAdvancedEq();

    connect(m_transport, &Transport::opened, this, [this] {
        setState(Connected);
        m_settings->setLastDeviceAddress(m_address);
        m_settings->setLastDeviceName(m_bluetoothName);
        refresh();
        m_batteryPoll.start();
    });
    connect(m_transport, &Transport::closed, this, [this] {
        if (m_state != Disconnected)
            setState(Disconnected);
    });
    connect(m_transport, &Transport::failed, this, [this](const QString &message) {
        setState(Disconnected, message);
    });
    connect(m_transport, &Transport::received, this, &Device::onBytes);

    m_timeout.setSingleShot(true);
    m_timeout.setInterval(ReplyTimeoutMs);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        // No reply: some commands are silently ignored by some firmware.
        // Move on; the model-id read gates the feature reads, so continue
        // those with the name-based guess.
        if (m_inFlight && m_inFlight->cmd == ModelIdRead && !m_featuresRead)
            readFeatures();
        m_inFlight.reset();
        pump();
    });

    // The earbuds push battery changes, but not reliably while idle.
    m_batteryPoll.setInterval(60 * 1000);
    connect(&m_batteryPoll, &QTimer::timeout, this, [this] { read(BatteryRead); });

    m_customEqDebounce.setSingleShot(true);
    m_customEqDebounce.setInterval(250);
    connect(&m_customEqDebounce, &QTimer::timeout, this, &Device::sendCustomEq);
    m_advancedEqDebounce.setSingleShot(true);
    m_advancedEqDebounce.setInterval(250);
    connect(&m_advancedEqDebounce, &QTimer::timeout, this, &Device::sendAdvancedEq);
}

Device::~Device() { m_transport->close(); }

QStringList Device::features() const {
    QStringList out(m_model.features.begin(), m_model.features.end());
    out.sort();
    return out;
}

QVariantList Device::pairedDevices() const {
    QVariantList out;
    for (const auto &d : m_paired)
        out.append(QVariantMap{{"address", d.address}, {"name", d.name}, {"thisHost", d.thisHost}, {"connected", d.connected}});
    return out;
}

QVariantList Device::gestures() const {
    QVariantList out;
    for (const auto &g : m_gestures)
        out.append(QVariantMap{{"device", g.device}, {"type", g.type}, {"action", g.action}});
    return out;
}

QVariantList Device::advancedBands() const {
    QVariantList out;
    for (const auto &b : m_bands)
        out.append(QVariantMap{{"gain", b.gain}, {"frequency", b.frequency}, {"q", b.q}});
    return out;
}

quint8 Device::ancToByte(int mode) {
    switch (mode) {
    case AncTransparency: return AncTransparencyByte;
    case AncAdaptive: return AncAdaptiveByte;
    case AncHigh: return AncHighByte;
    case AncMid: return AncMidByte;
    case AncLow: return AncLowByte;
    default: return AncOffByte;
    }
}

int Device::ancFromByte(quint8 byte) {
    switch (byte) {
    case AncTransparencyByte: return AncTransparency;
    case AncAdaptiveByte: return AncAdaptive;
    case AncHighByte: return AncHigh;
    case AncMidByte: return AncMid;
    case AncLowByte: return AncLow;
    default: return AncOff;
    }
}

void Device::setState(State state, const QString &error) {
    if (state == Disconnected) {
        m_queue.clear();
        m_inFlight.reset();
        m_timeout.stop();
        m_batteryPoll.stop();
        m_battery = {};
        m_values.remove("ringingLeft");
        m_values.remove("ringingRight");
        emit batteryChanged();
    }
    if (state == m_state && error == m_error)
        return;
    m_state = state;
    m_error = error;
    emit stateChanged();
}

void Device::setValue(const QString &key, const QVariant &value) {
    if (m_values.value(key) == value)
        return;
    m_values.insert(key, value);
    emit valuesChanged();
}

// --- Connection ---------------------------------------------------------------

void Device::connectToDevice(const QString &address, const QString &name) {
    if (address.isEmpty())
        return;
    if (m_state != Disconnected)
        m_transport->close();
    m_address = address;
    m_bluetoothName = name;
    m_values.clear();
    m_gestures.clear();
    m_paired.clear();
    m_featuresRead = false;
    m_decoder = {};
    applyModel(Models::byName(name));
    setState(Connecting);
    m_transport->open(address);
}

void Device::connectToLast() {
    connectToDevice(m_settings->lastDeviceAddress(), m_settings->lastDeviceName());
}

void Device::disconnectFromDevice() {
    m_transport->close();
    setState(Disconnected);
}

void Device::applyModel(const Model &model) {
    m_model = model;
    emit modelChanged();
}

void Device::refresh() {
    if (m_state != Connected)
        return;
    m_featuresRead = false;
    // Model first: what we read next depends on it. Then the session
    // handshake the Nothing X app does, then the essentials.
    read(ModelIdRead);
    read(ProtocolVersionRead);
    enqueue(Activate);
    read(BatteryRead);
    read(AncRead);
    read(FirmwareRead);
    read(DeviceInfoRead);
    enqueue(TimeWrite, timePayload(QDateTime::currentSecsSinceEpoch()));
}

void Device::readFeatures() {
    if (m_featuresRead)
        return;
    m_featuresRead = true;
    const Model &m = m_model;
    if (m.has("eq")) {
        read(EqPresetRead);
        if (m.has("customEq")) read(CustomEqRead);
        if (m.has("advancedEq")) {
            read(AdvancedEqStatusRead);
            read(AdvancedEqBandsRead, QByteArray(1, char(0xFF)));
        }
    }
    if (m.has("inEar")) read(InEarRead);
    if (m.has("lowLatency")) read(LowLatencyRead);
    if (m.has("gestures")) read(GesturesRead);
    if (m.has("bass")) read(BassRead);
    if (m.has("dualConnection")) {
        read(DualEnableRead);
        read(DualListRead, QByteArray(1, 0));
    }
    if (m.has("ldac")) read(CodecRead);
    if (m.has("spatial")) read(SpatialRead);
    if (m.has("personalSound")) read(PersonalSoundRead);
    if (m.has("superMic")) read(SuperMicRead);
    if (m.has("micMode")) read(MicModeRead);
    if (m.has("callTransparency")) read(CallTransparencyRead);
    if (m.has("walkieTalkie")) read(WalkieTalkieRead);
    read(WearStateRead);
}

// --- Queue --------------------------------------------------------------------

void Device::enqueue(quint16 cmd, const QByteArray &payload) {
    if (m_state != Connected)
        return;
    // A read already waiting covers this one.
    if ((cmd & 0xF000) == 0xC000)
        for (const auto &r : std::as_const(m_queue))
            if (r.cmd == cmd && r.payload == payload)
                return;
    m_queue.enqueue({cmd, payload});
    pump();
}

void Device::write(quint16 cmd, const QByteArray &payload, quint16 readBack) {
    // A newer write of the same setting replaces one still waiting.
    for (int i = 0; i < m_queue.size(); ++i) {
        if (m_queue[i].cmd == cmd && cmd != GesturesWrite && cmd != FindBuds && cmd != DualConnectWrite) {
            m_queue.removeAt(i);
            break;
        }
    }
    enqueue(cmd, payload);
    if (readBack)
        enqueue(readBack);
}

void Device::pump() {
    if (m_inFlight || m_queue.isEmpty() || !m_transport->isOpen())
        return;
    m_inFlight = m_queue.dequeue();
    m_fsn = m_fsn >= 253 ? 1 : m_fsn + 1;
    m_transport->write(encode(m_inFlight->cmd, m_inFlight->payload, m_fsn));
    m_timeout.start();
}

void Device::onBytes(const QByteArray &bytes) {
    m_decoder.feed(bytes);
    while (auto frame = m_decoder.next())
        handle(*frame);
}

// --- Replies and pushes ---------------------------------------------------------

void Device::handle(const Frame &f) {
    const QByteArray &p = f.payload;
    auto u8 = [&p](int i) { return i < p.size() ? quint8(p[i]) : quint8(0); };

    switch (f.cmd) {
    case replyTo(ModelIdRead): {
        const QString id = parseModelId(p);
        if (!id.isEmpty()) {
            Model model = Models::byId(id);
            // Unknown ids keep the name-based guess rather than a bare generic.
            if (model.name == QLatin1String("Nothing earbuds"))
                model.name = m_model.name;
            applyModel(model);
        }
        readFeatures();
        break;
    }
    case replyTo(BatteryRead):
    case BatteryPush:
        m_battery = parseBattery(p);
        emit batteryChanged();
        break;
    case replyTo(AncRead):
    case AncPush:
        if (auto anc = parseAnc(p)) {
            m_ancMode = ancFromByte(anc->mode);
            if (isNoiseCancelling(m_ancMode))
                m_lastNcMode = m_ancMode;
            emit ancChanged();
        }
        break;
    case replyTo(FirmwareRead):
        setValue("firmware", QString::fromLatin1(p).trimmed());
        break;
    case replyTo(DeviceInfoRead): {
        const DeviceInfo info = parseDeviceInfo(p);
        if (!info.serial.isEmpty()) setValue("serial", info.serial);
        if (!info.caseFirmware.isEmpty()) setValue("caseFirmware", info.caseFirmware);
        break;
    }
    case replyTo(EqPresetRead):
        setValue("eqPreset", int(u8(0)));
        break;
    case replyTo(CustomEqRead):
        if (auto eq = parseCustomEq(p)) {
            m_customEq = *eq;
            setValue("customEq", QVariantList{eq->bass, eq->mid, eq->treble});
        }
        break;
    case replyTo(AdvancedEqStatusRead):
        setValue("advancedEq", u8(0) == 1);
        break;
    case replyTo(AdvancedEqBandsRead): {
        const auto bands = parseAdvancedEq(p);
        if (!bands.isEmpty()) {
            m_bands = bands;
            emit valuesChanged();
        }
        break;
    }
    case replyTo(BassRead):
        setValue("bassEnhance", u8(0) == 1);
        setValue("bassLevel", qBound(1, int(u8(1)) / 2, 5));
        break;
    case replyTo(SpatialRead):
        setValue("spatial", int(u8(0)));
        break;
    case replyTo(PersonalSoundRead):
        setValue("personalSound", u8(0) == 1);
        break;
    case PersonalSoundPush:
        read(PersonalSoundRead);
        break;
    case replyTo(InEarRead):
        if (auto on = parseInEar(p))
            setValue("inEarDetection", *on);
        break;
    case replyTo(WearStateRead): {
        const WearState w = parseWearState(p);
        if (w.left >= 0) setValue("leftInEar", w.leftInEar());
        if (w.right >= 0) setValue("rightInEar", w.rightInEar());
        break;
    }
    case WearStatePush:
        // Case opened or closed, an earbud put in or taken out.
        read(WearStateRead);
        read(BatteryRead);
        break;
    case replyTo(LowLatencyRead):
        setValue("lowLatency", u8(0) == 1);
        break;
    case replyTo(CodecRead):
        setValue("codec", int(u8(0)));
        break;
    case replyTo(DualEnableRead):
    case DualEnablePush:
        setValue("dualConnection", u8(0) == 1);
        break;
    case replyTo(DualListRead): {
        int reported = 0;
        const auto page = parseDualList(p, &reported);
        for (const auto &d : page) {
            auto it = std::find_if(m_paired.begin(), m_paired.end(), [&](const PairedDevice &e) { return e.address == d.address; });
            if (it != m_paired.end()) *it = d;
            else m_paired.append(d);
        }
        // The list is paged; ask for the next page while one may remain.
        if (!page.isEmpty() && page.size() < reported && m_paired.size() < 20)
            read(DualListRead, QByteArray(1, char(m_paired.size())));
        emit valuesChanged();
        break;
    }
    case DualDevicePush:
        if (auto d = parseDualDevicePush(p)) {
            auto it = std::find_if(m_paired.begin(), m_paired.end(), [&](const PairedDevice &e) { return e.address == d->address; });
            if (it != m_paired.end()) {
                it->connected = d->connected;
                emit valuesChanged();
            } else {
                refreshPairedDevices();
            }
        }
        break;
    case replyTo(GesturesRead):
        m_gestures = parseGestures(p);
        emit valuesChanged();
        break;
    case FitTestPush:
        if (auto r = parseFitResult(p)) {
            setValue("fitRunning", false);
            setValue("fitLeft", r->left);
            setValue("fitRight", r->right);
            emit fitTestFinished(r->left, r->right);
        }
        break;
    case replyTo(SuperMicRead):
        setValue("superMic", u8(0) == 1);
        break;
    case replyTo(MicModeRead):
        setValue("micMode", int(u8(0)));
        break;
    case replyTo(CallTransparencyRead):
        setValue("callTransparency", u8(0) == 1);
        break;
    case replyTo(WalkieTalkieRead):
        setValue("walkieTalkie", u8(0) == 1);
        break;
    default:
        break;
    }

    // Replies complete the request in flight; pushes don't.
    if (m_inFlight && f.cmd == replyTo(m_inFlight->cmd)) {
        m_timeout.stop();
        m_inFlight.reset();
        pump();
    }
}

// --- Settings -----------------------------------------------------------------

void Device::setAncMode(int mode) {
    m_ancMode = mode;
    if (isNoiseCancelling(mode))
        m_lastNcMode = mode;
    emit ancChanged();
    write(AncWrite, ancPayload(ancToByte(mode)), AncRead);
}

void Device::setEqPreset(int preset) {
    setValue("eqPreset", preset);
    setValue("advancedEq", false);
    if (m_model.has("advancedEq"))
        write(AdvancedEqStatusWrite, QByteArray::fromHex("0000"));
    write(EqPresetWrite, QByteArray(1, char(preset)) + char(0), EqPresetRead);
    if (preset == EqCustom && m_model.has("customEq"))
        read(CustomEqRead);
}

void Device::setAdvancedEq(bool on) {
    setValue("advancedEq", on);
    write(AdvancedEqStatusWrite, QByteArray(1, char(on ? 1 : 0)) + char(0), AdvancedEqStatusRead);
    if (on)
        read(AdvancedEqBandsRead, QByteArray(1, char(0xFF)));
    else
        read(EqPresetRead);
}

void Device::setCustomEq(double bass, double mid, double treble) {
    m_customEq = {qBound(-6.0, bass, 6.0), qBound(-6.0, mid, 6.0), qBound(-6.0, treble, 6.0)};
    setValue("customEq", QVariantList{m_customEq.bass, m_customEq.mid, m_customEq.treble});
    m_customEqDebounce.start();
}

void Device::sendCustomEq() {
    if (m_values.value("eqPreset").toInt() != EqCustom || m_values.value("advancedEq").toBool()) {
        setValue("eqPreset", int(EqCustom));
        setValue("advancedEq", false);
        if (m_model.has("advancedEq"))
            write(AdvancedEqStatusWrite, QByteArray::fromHex("0000"));
        write(EqPresetWrite, QByteArray(1, char(EqCustom)) + char(0));
    }
    write(CustomEqWrite, customEqPayload(m_customEq));
}

void Device::setAdvancedBandGain(int index, double gain) {
    if (index < 0 || index >= m_bands.size())
        return;
    m_bands[index].gain = qBound(-6.0, std::round(gain * 2.0) / 2.0, 6.0);
    emit valuesChanged();
    m_advancedEqDebounce.start();
}

void Device::resetAdvancedEq() {
    m_bands = defaultAdvancedEq();
    emit valuesChanged();
    sendAdvancedEq();
}

void Device::sendAdvancedEq() {
    if (!m_values.value("advancedEq").toBool()) {
        setValue("advancedEq", true);
        write(AdvancedEqStatusWrite, QByteArray::fromHex("0100"));
    }
    write(AdvancedEqBandsWrite, advancedEqPayload(m_bands));
}

void Device::setBass(bool on, int level) {
    level = qBound(1, level, 5);
    setValue("bassEnhance", on);
    setValue("bassLevel", level);
    write(BassWrite, QByteArray(1, char(on ? 1 : 0)) + char(level * 2), BassRead);
}

void Device::setSpatial(int mode) {
    setValue("spatial", mode);
    write(SpatialWrite, QByteArray(1, char(mode)) + char(0), SpatialRead);
}

void Device::setPersonalSound(bool on) {
    setValue("personalSound", on);
    write(PersonalSoundWrite, QByteArray(1, char(on ? 1 : 0)), PersonalSoundRead);
}

void Device::setInEarDetection(bool on) {
    setValue("inEarDetection", on);
    write(InEarWrite, QByteArray::fromHex("0101") + char(on ? 1 : 0), InEarRead);
}

void Device::setLowLatency(bool on) {
    setValue("lowLatency", on);
    write(LowLatencyWrite, QByteArray(1, char(on ? 1 : 2)) + char(0), LowLatencyRead);
}

void Device::setCodec(int codec) {
    setValue("codec", codec);
    // The earbuds restart to switch codec, which drops this connection; the
    // auto-reconnect picks them up again.
    write(CodecWrite, QByteArray(1, char(codec)));
}

void Device::setDualConnection(bool on) {
    setValue("dualConnection", on);
    write(DualEnableWrite, QByteArray(1, char(on ? 1 : 0)), DualEnableRead);
    if (on)
        refreshPairedDevices();
}

void Device::refreshPairedDevices() {
    m_paired.clear();
    emit valuesChanged();
    read(DualListRead, QByteArray(1, 0));
}

void Device::connectPairedDevice(const QString &address, bool connect) {
    write(DualConnectWrite, dualConnectPayload(address, connect));
}

void Device::setSuperMic(bool on) {
    setValue("superMic", on);
    write(SuperMicWrite, QByteArray(1, char(on ? 1 : 0)), SuperMicRead);
}

void Device::setMicMode(int mode) {
    setValue("micMode", mode);
    write(MicModeWrite, QByteArray(1, char(mode)), MicModeRead);
}

void Device::setCallTransparency(bool on) {
    setValue("callTransparency", on);
    write(CallTransparencyWrite, QByteArray(1, char(on ? 1 : 0)), CallTransparencyRead);
}

void Device::setWalkieTalkie(bool on) {
    setValue("walkieTalkie", on);
    write(WalkieTalkieWrite, QByteArray(1, char(on ? 1 : 0)), WalkieTalkieRead);
}

int Device::gestureAction(int device, int type) const {
    for (const auto &g : m_gestures)
        if (g.device == device && g.type == type)
            return g.action;
    return -1;
}

void Device::setGesture(int device, int type, int action) {
    Gesture g{quint8(device), 1, quint8(type), quint8(action)};
    bool found = false;
    for (auto &e : m_gestures)
        if (e.device == device && e.type == type) {
            e.action = quint8(action);
            found = true;
        }
    if (!found)
        m_gestures.append(g);
    emit valuesChanged();
    write(GesturesWrite, gesturePayload(g));
    // Read back once the last change in a burst has gone out.
    for (const auto &r : std::as_const(m_queue))
        if (r.cmd == GesturesRead)
            return;
    enqueue(GesturesRead);
}

void Device::startFitTest() {
    setValue("fitRunning", true);
    setValue("fitLeft", -1);
    setValue("fitRight", -1);
    write(FitTestStart, QByteArray(1, 1));
    // The result is pushed when the test tone ends (about 10 s); give up if
    // it never comes.
    QTimer::singleShot(25000, this, [this] { setValue("fitRunning", false); });
}

void Device::ring(int side, bool on) {
    setValue(side == Left ? "ringingLeft" : "ringingRight", on);
    write(FindBuds, QByteArray(1, char(side)) + char(on ? 1 : 0));
}
