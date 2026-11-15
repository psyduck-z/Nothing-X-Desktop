#include "device.h"
#include "officialpack.h"
#include "settings.h"
#include "transport.h"

#include <QDateTime>
#include <QJsonArray>
#include <algorithm>

using namespace Proto;

namespace {
constexpr int ReplyTimeoutMs = 4000;
}

Device::Device(Transport *transport, Settings *settings, OfficialPack *pack, QObject *parent)
    : QObject(parent), m_transport(transport), m_settings(settings), m_pack(pack) {
    m_transport->setParent(this);
    m_bands = defaultAdvancedBands();

    m_timeout.setSingleShot(true);
    m_timeout.setInterval(ReplyTimeoutMs);
    connect(&m_timeout, &QTimer::timeout, this, [this] {
        if (!m_inFlight)
            return;
        Request request = *m_inFlight;
        m_inFlight.reset();
        if (request.retries > 0) {
            --request.retries;
            m_queue.prepend(request);
        } else if (request.cmd == Cmd::FirmwareRead && m_state == Connecting) {
            // The official app gives up when the first read never answers.
            disconnectFromDevice();
            setState(Disconnected, tr("The earbuds didn't answer. Take them out of the case and try again."));
            return;
        }
        pump();
    });
    // A frame whose CRC bytes never came: take it as it is once the line is quiet.
    m_flush.setSingleShot(true);
    m_flush.setInterval(80);
    connect(&m_flush, &QTimer::timeout, this, [this] { onBytes({}); });
    m_batteryPoll.setInterval(5 * 60 * 1000);
    connect(&m_batteryPoll, &QTimer::timeout, this, [this] { read(Cmd::BatteryRead); });
    m_customEqDebounce.setSingleShot(true);
    m_customEqDebounce.setInterval(250);
    connect(&m_customEqDebounce, &QTimer::timeout, this, &Device::sendCustomEq);
    m_advancedEqDebounce.setSingleShot(true);
    m_advancedEqDebounce.setInterval(250);
    connect(&m_advancedEqDebounce, &QTimer::timeout, this, &Device::sendAdvancedEq);
    // Ringing stops by itself after a minute, as in the official app.
    for (auto [timer, side] : {std::pair{&m_ringLeft, int(PartLeft)}, std::pair{&m_ringRight, int(PartRight)}}) {
        timer->setSingleShot(true);
        timer->setInterval(60 * 1000);
        connect(timer, &QTimer::timeout, this, [this, side] { stopRing(side); });
    }
    m_fitTimeout.setSingleShot(true);
    connect(&m_fitTimeout, &QTimer::timeout, this, [this] {
        if (m_values.value("fitRunning").toBool()) {
            setValue("fitRunning", false);
            setValue("fitLeft", -2);
            setValue("fitRight", -2);
            emit fitTestFinished(-2, -2);
        }
    });

    connect(m_transport, &Transport::opened, this, [this] {
        m_decoder.clear();
        // First the firmware version, retried: the official app's refresh
        // starts there and stops if it never answers.
        enqueue({Cmd::FirmwareRead, {}, [this](const Frame &f) {
                     if (m_state != Connecting)
                         return;
                     if (!f.ok() && f.payload.isEmpty()) {
                         setState(Disconnected, tr("The earbuds refused the connection."));
                         return;
                     }
                     setState(Connected);
                     m_settings->setLastDeviceAddress(m_address);
                     m_settings->setLastDeviceName(m_bluetoothName);
                     readAll();
                 },
                 2});
    });
    connect(m_transport, &Transport::closed, this, [this] {
        m_queue.clear();
        m_inFlight.reset();
        m_timeout.stop();
        m_batteryPoll.stop();
        setState(Disconnected);
    });
    connect(m_transport, &Transport::failed, this, [this](const QString &message) {
        m_queue.clear();
        m_inFlight.reset();
        setState(Disconnected, message);
    });
    connect(m_transport, &Transport::received, this, &Device::onBytes);
    if (m_pack)
        connect(m_pack, &OfficialPack::changed, this, [this] {
            if (!m_model.id.isEmpty())
                applyModel(m_model.id);
        });
}

Device::~Device() { m_transport->disconnect(this); }

int Device::batteryCase() const { return m_battery.caseUnit.level >= 0 ? m_battery.caseUnit.level : m_caseLast; }

QString Device::displayName() const {
    if (!m_model.name.isEmpty())
        return m_model.name;
    QString name = m_bluetoothName;
    return name.remove(QStringLiteral("Nothing ")).trimmed();
}

// --- Connection -----------------------------------------------------------------

void Device::setState(State state, const QString &error) {
    if (m_state == state && m_error == error)
        return;
    m_state = state;
    m_error = error;
    if (state == Connected)
        m_batteryPoll.start();
    emit stateChanged();
}

void Device::connectToDevice(const QString &address, const QString &name) {
    if (address.isEmpty())
        return;
    if (m_state != Disconnected)
        m_transport->close();
    m_address = address;
    m_bluetoothName = name;
    m_battery = {};
    m_wear = {};
    m_caseLast = m_settings->deviceValue(address, "caseBattery", -1).toInt();
    m_values.clear();
    m_paired.clear();
    m_gestures.clear();
    m_bands = defaultAdvancedBands();
    applyModel(ModelConfig::idFromName(name));
    setState(Connecting);
    emit batteryChanged();
    emit valuesChanged();
    m_transport->open(address);
}

void Device::connectToLast() { connectToDevice(m_settings->lastDeviceAddress(), m_settings->lastDeviceName()); }

void Device::disconnectFromDevice() {
    m_queue.clear();
    m_inFlight.reset();
    m_timeout.stop();
    m_transport->close();
    setState(Disconnected);
}

void Device::applyModel(const QString &reportedId) {
    // The Ear (3) reports B201, the id the official list files it under is B173.
    QString id = m_pack ? m_pack->productId(reportedId) : reportedId;
    if (id == QLatin1String("B201"))
        id = QStringLiteral("B173");
    ModelConfig model;
    if (m_pack && !id.isEmpty()) {
        const QJsonObject entry = m_pack->modelConfig(id);
        if (!entry.isEmpty()) {
            QString name;
            const QJsonArray skus = m_pack->modelSkus(id);
            if (!skus.isEmpty())
                name = skus.first().toObject().value("deviceSpu").toObject().value("name").toString();
            model = ModelConfig::fromOfficial(entry, name);
        }
    }
    if (!model.valid() && (id == QLatin1String("B173") || id.isEmpty()))
        model = ModelConfig::builtInEar3();
    if (!model.valid())
        model = ModelConfig::builtInEar3();
    m_model = model;
    emit modelChanged();
}

void Device::refresh() {
    if (m_state == Connected)
        readAll();
}

void Device::readAll() {
    read(Cmd::ModelRead);
    read(Cmd::ColorRead);
    read(Cmd::BatteryRead);
    read(Cmd::StatusRead);
    read(Cmd::InfoRead);
    read(Cmd::CaseFirmwareRead);
    if (has("anc"))
        read(Cmd::AncRead, QByteArray(1, 3));
    if (has("eq")) {
        read(Cmd::EqModeRead);
        read(Cmd::CustomEqRead);
    }
    if (has("advancedEq")) {
        read(Cmd::AdvancedEqModeRead);
        read(Cmd::AdvancedEqRead, le16Bytes(0));
    }
    if (has("ultraBass"))
        read(Cmd::BassRead);
    if (has("spatial")) {
        read(Cmd::SpatialRead);
        read(Cmd::MutualExclusionRead);
    }
    if (has("ldac"))
        read(Cmd::LdacRead);
    if (has("lowLag"))
        read(Cmd::LowLagRead);
    if (has("inEar"))
        read(Cmd::InEarRead);
    if (has("gestures"))
        read(Cmd::GesturesRead);
    if (has("personalSound"))
        read(Cmd::PersonalSoundRead);
    if (has("dual")) {
        read(Cmd::DualRead);
        read(Cmd::DualListRead, le16Bytes(0));
    }
    if (has("superMic"))
        read(Cmd::SuperMicRead);
    if (has("callTransparency"))
        read(Cmd::CallTransparencyRead);
    if (has("walkieTalkie"))
        read(Cmd::WalkieTalkieRead);
}

// --- Command queue --------------------------------------------------------------

void Device::enqueue(Request request) {
    m_queue.enqueue(std::move(request));
    pump();
}

void Device::read(quint16 cmd, const QByteArray &payload) {
    // A read already waiting would answer the same question.
    for (const Request &r : m_queue)
        if (r.cmd == cmd && r.payload == payload && !r.onReply)
            return;
    enqueue({cmd, payload, {}, 0});
}

void Device::write(quint16 cmd, const QByteArray &payload, const std::function<void()> &undo) {
    if (m_state != Connected)
        return;
    enqueue({cmd, payload, [this, undo](const Frame &f) {
                 if (f.ok())
                     return;
                 emit notice(statusText(f.status()));
                 if (undo)
                     undo();
             },
             0});
}

void Device::pump() {
    if (m_inFlight || m_queue.isEmpty() || !m_transport->isOpen())
        return;
    m_inFlight = m_queue.dequeue();
    m_transport->write(encode(m_inFlight->cmd, m_inFlight->payload, m_fsn.next()));
    m_timeout.start();
}

void Device::onBytes(const QByteArray &bytes) {
    // NOTHINGX_LOG_FRAMES=1 logs every frame from the earbuds (debugging).
    static const bool logFrames = qEnvironmentVariableIsSet("NOTHINGX_LOG_FRAMES");
    if (logFrames && !bytes.isEmpty())
        qInfo().noquote() << "nothingx rx" << bytes.toHex(' ');
    m_decoder.feed(bytes);
    const bool flush = bytes.isEmpty();
    while (auto frame = m_decoder.next(flush))
        handle(*frame);
    if (m_decoder.pending())
        m_flush.start();
}

QString Device::statusText(int status) {
    switch (status) {
    case NotSupported: return tr("The earbuds don't support that.");
    case InvalidParameter: return tr("The earbuds didn't accept that value.");
    case NotActivated: return tr("The earbuds aren't ready yet. Try again in a moment.");
    default: return tr("The earbuds didn't apply that change.");
    }
}

void Device::setValue(const QString &key, const QVariant &value) {
    if (m_values.value(key) == value && m_values.contains(key))
        return;
    m_values.insert(key, value);
    emit valuesChanged();
}

// --- Incoming frames -------------------------------------------------------------

void Device::handle(const Frame &f) {
    // The earbuds ask for the time; answer at once, like the official app.
    if (f.cmd == Cmd::TimeRequest) {
        m_transport->write(encode(Cmd::TimeWrite, timePayload(QDateTime::currentSecsSinceEpoch()), m_fsn.next()));
        return;
    }
    // The case's TALK button asked for an Essential Space recording, which
    // only Nothing phones can make: say it isn't available here.
    if (f.cmd == Cmd::EssentialEvent) {
        m_transport->write(encode(Cmd::EssentialReply, QByteArray(1, 3), m_fsn.next()));
        return;
    }

    std::optional<Request> answered;
    if (m_inFlight && m_inFlight->cmd == f.cmd && !f.isNotification()) {
        m_timeout.stop();
        answered = m_inFlight;
        m_inFlight.reset();
    }

    const QByteArray &p = f.payload;
    const bool data = f.isNotification() || (f.isRead() && f.ok());
    if (data) {
        auto u8 = [&p](int i) { return i < p.size() ? quint8(p[i]) : 0; };
        switch (f.cmd) {
        case Cmd::FirmwareRead:
            if (const QString v = parseFirmware(p); !v.isEmpty())
                setValue("firmware", v);
            break;
        case Cmd::CaseFirmwareRead:
            if (const QString v = parseCaseFirmware(p); !v.isEmpty())
                setValue("caseFirmware", v);
            break;
        case Cmd::InfoRead: {
            const DeviceInfo info = parseDeviceInfo(p);
            if (!info.serial.isEmpty())
                setValue("serial", info.serial);
            if (!info.caseFirmware.isEmpty() && !m_values.contains("caseFirmware"))
                setValue("caseFirmware", info.caseFirmware);
            break;
        }
        case Cmd::ModelRead:
            if (const QString id = parseModelId(p); !id.isEmpty() && id != m_model.id)
                applyModel(id);
            break;
        case Cmd::ColorRead:
            if (const QString c = parseColorId(p); c != m_colorId) {
                m_colorId = c;
                emit modelChanged();
            }
            break;
        case Cmd::BatteryRead:
        case Cmd::BatteryEvent: {
            const int caseBefore = m_battery.caseUnit.level;
            mergeBattery(m_battery, p);
            if (m_battery.caseUnit.level >= 0 && m_battery.caseUnit.level != caseBefore) {
                m_caseLast = m_battery.caseUnit.level;
                m_settings->setDeviceValue(m_address, "caseBattery", m_caseLast);
            }
            emit batteryChanged();
            break;
        }
        case Cmd::StatusRead:
        case Cmd::StatusEvent: {
            const int caseBefore = m_wear.caseUnit;
            mergeWear(m_wear, p);
            emit batteryChanged();
            // The lid opened or a bud went in or out: the case may report now.
            if (f.cmd == Cmd::StatusEvent && m_wear.caseUnit != caseBefore)
                read(Cmd::BatteryRead);
            break;
        }
        case Cmd::AncRead:
        case Cmd::AncEvent: {
            const AncState anc = parseAnc(p);
            if (anc.mode >= 0)
                m_ancMode = anc.mode == 0 ? AncOff : anc.mode == 254 ? AncTransparency : anc.mode;
            if (anc.lastLevel >= AncHigh && anc.lastLevel <= AncAdaptive)
                m_ancLevel = anc.lastLevel;
            else if (m_ancMode >= AncHigh && m_ancMode <= AncAdaptive)
                m_ancLevel = m_ancMode;
            emit ancChanged();
            break;
        }
        case Cmd::EqModeRead:
            setValue("eqPreset", leInt(p));
            break;
        case Cmd::CustomEqRead:
            if (auto eq = parseCustomEq(p)) {
                m_customEq = *eq;
                setValue("customBass", eq->bass);
                setValue("customMid", eq->mid);
                setValue("customTreble", eq->treble);
            }
            break;
        case Cmd::AdvancedEqModeRead:
            setValue("advancedEq", u8(0) == 1);
            break;
        case Cmd::AdvancedEqRead:
            // No bands yet means factory defaults (the app's "isInitializer").
            if (const auto bands = parseAdvancedEq(p); !bands.isEmpty()) {
                m_bands = bands;
                emit valuesChanged();
            }
            break;
        case Cmd::BassRead: {
            const Bass b = parseBass(p);
            setValue("bassOn", b.on);
            setValue("bassLevel", b.level);
            break;
        }
        case Cmd::SpatialRead:
            setValue("spatial", parseSpatial(p));
            break;
        case Cmd::MutualExclusionRead:
            setValue("mutuallyExclusive", u8(0) == 1);
            break;
        case Cmd::LdacRead:
            setValue("ldac", leInt(p) == 2);
            break;
        case Cmd::LowLagRead:
            setValue("lowLag", leInt(p) != 2 && !p.isEmpty());
            break;
        case Cmd::InEarRead:
            if (auto on = parseInEar(p))
                setValue("inEar", *on);
            break;
        case Cmd::GesturesRead:
            m_gestures = parseGestures(p);
            emit valuesChanged();
            break;
        case Cmd::PersonalSoundRead:
            setValue("personalSound", u8(0) == 1);
            break;
        case Cmd::PersonalSoundEvent:
            read(Cmd::PersonalSoundRead);
            break;
        case Cmd::DualRead:
            setValue("dual", leInt(p) != 0);
            break;
        case Cmd::DualListRead: {
            const DualPage page = parseDualList(p);
            if (page.page == 0)
                m_paired.clear();
            m_paired.append(page.devices);
            emit valuesChanged();
            if (page.page + 1 < page.pages)
                read(Cmd::DualListRead, le16Bytes(page.page + 1));
            break;
        }
        case Cmd::DualSwitchEvent:
            read(Cmd::DualRead);
            break;
        case Cmd::DualDeviceEvent:
            refreshPairedDevices();
            break;
        case Cmd::SuperMicRead:
            setValue("superMic", u8(0) == 1);
            break;
        case Cmd::CallTransparencyRead:
            setValue("callTransparency", u8(0) == 1);
            break;
        case Cmd::WalkieTalkieRead:
            setValue("walkieTalkie", u8(0) == 1);
            break;
        case Cmd::GestureEvent:
            // The action the official app uses for News Reporter: the earbuds
            // send it over this link, so the desktop can act on it.
            emit gestureEvent(leInt(p));
            break;
        case Cmd::FitTestEvent:
            if (p.size() >= 2) {
                m_fitTimeout.stop();
                setValue("fitRunning", false);
                setValue("fitLeft", int(u8(0)));
                setValue("fitRight", int(u8(1)));
                emit fitTestFinished(u8(0), u8(1));
            }
            break;
        default:
            break;
        }
    }
    if (answered) {
        if (answered->onReply)
            answered->onReply(f);
        pump();
    }
}

// --- Data for QML ------------------------------------------------------------------

QVariantList Device::advancedBands() const {
    QVariantList out;
    for (const EqBand &b : m_bands)
        out.append(QVariantMap{{"frequency", b.frequency}, {"gain", b.gain}, {"q", b.q}});
    return out;
}

QVariantList Device::gestures() const {
    QVariantList out;
    for (const Gesture &g : m_gestures)
        out.append(QVariantMap{{"device", g.device}, {"button", g.button}, {"gesture", g.gesture}, {"action", g.action}});
    return out;
}

QVariantList Device::pairedDevices() const {
    QVariantList out;
    for (const PairedDevice &d : m_paired)
        out.append(QVariantMap{{"address", d.address}, {"name", d.name}, {"connected", d.connected}, {"thisHost", d.thisHost}});
    return out;
}

int Device::gestureAction(int device, int gesture) const {
    for (const Gesture &g : m_gestures)
        if (g.device == device && g.gesture == gesture)
            return g.action;
    return -1;
}

QString Device::conflict(const QString &setting, const QVariant &value) const {
    const bool spatialOn = m_values.value("spatial").toInt() > 0;
    const bool ldac = m_values.value("ldac").toBool();
    const bool spaceOrAdvance = has("spaceOrAdvance");
    const bool exclusive = m_values.value("mutuallyExclusive").toBool();
    // The official app's own wording for each case.
    if (setting == "spatial" && value.toInt() > 0) {
        if (ldac)
            return tr("Spatial Audio is not compatible with LDAC. Turning on Spatial Audio will turn off LDAC.");
        const bool advanced = m_values.value("advancedEq").toBool();
        const bool bass = m_values.value("bassOn").toBool();
        if (spaceOrAdvance && advanced && bass)
            return tr("Bass enhance and Advanced equaliser is not compatible with Spatial audio. Are you sure to enable "
                      "Spatial audio and turn off both Bass enhance and Advanced equaliser?");
        if (spaceOrAdvance && bass)
            return tr("Bass enhancement is not compatible with spatial audio. Are you sure you want to enable spatial "
                      "audio and turn off bass enhancement?");
        if (spaceOrAdvance && advanced)
            return tr("Advanced equaliser is not compatible with Spatial audio. Are you sure to enable Advanced "
                      "equaliser and turn off Spatial audio?");
        if (exclusive && m_values.value("eqPreset").toInt() != 0)
            return tr("Spatial audio is not compatible with Equaliser. Enabling spatial audio will set the Equaliser to "
                      "\"Balanced\" mode. Are you sure?");
        if (m_ancMode == AncAdaptive)
            return tr("Adaptive ANC is not compatible with Spatial Audio. Turning on Adaptive ANC will turn off Spatial Audio.");
    } else if (setting == "ldac" && value.toBool()) {
        if (spatialOn)
            return tr("LDAC is not compatible with Spatial Audio. Turning on LDAC will turn off Spatial Audio.");
        if (m_ancMode == AncAdaptive)
            return tr("Adaptive ANC is not compatible with LDAC. Turning on Adaptive ANC will turn off LDAC.");
        return tr("This codec may reduce battery life. Interference and compatibility issues may occur, which could "
                  "result in choppy audio.");
    } else if (setting == "anc" && value.toInt() == AncAdaptive) {
        if (ldac)
            return tr("Adaptive ANC is not compatible with LDAC. Turning on Adaptive ANC will turn off LDAC.");
        if (spatialOn)
            return tr("Adaptive ANC is not compatible with Spatial Audio. Turning on Adaptive ANC will turn off Spatial Audio.");
    } else if (setting == "personalSound" && value.toBool() && ldac) {
        return tr("Personal Sound is not compatible with LDAC. Turning on Personal Sound will turn off LDAC.");
    } else if (setting == "advancedEq" && value.toBool() && spatialOn && spaceOrAdvance) {
        return tr("Advanced equaliser is not compatible with Spatial audio. Are you sure to enable Advanced equaliser "
                  "and turn off Spatial audio?");
    } else if (setting == "bass" && value.toBool() && spatialOn && spaceOrAdvance) {
        return tr("Bass enhance is not compatible with Spatial audio. Are you sure to enable Bass enhance and turn off "
                  "Spatial audio?");
    } else if (setting == "eqPreset" && value.toInt() != 0 && spatialOn && exclusive) {
        return tr("Equaliser is not compatible with Spatial audio. Are you sure to enable equaliser and turn off Spatial audio?");
    }
    return {};
}

// --- Settings ----------------------------------------------------------------------

void Device::setAncMode(int mode) {
    if (mode == AncAdaptive) {
        if (m_values.value("ldac").toBool()) {
            setValue("ldac", false);
            write(Cmd::LdacWrite, le16Bytes(0), [this] { read(Cmd::LdacRead); });
        }
        if (m_values.value("spatial").toInt() > 0) {
            setValue("spatial", 0);
            write(Cmd::SpatialWrite, spatialPayload(0), [this] { read(Cmd::SpatialRead); });
        }
    }
    m_ancMode = mode;
    if (mode >= AncHigh && mode <= AncAdaptive)
        m_ancLevel = mode;
    emit ancChanged();
    write(Cmd::AncWrite, ancPayload(mode), [this] { read(Cmd::AncRead, QByteArray(1, 3)); });
    // The official app reads the mode back two seconds after a change.
    QTimer::singleShot(2000, this, [this] {
        if (m_state == Connected)
            read(Cmd::AncRead, QByteArray(1, 3));
    });
}

void Device::setNoiseCancellation() {
    int level = m_ancLevel;
    if (level < AncHigh || level > AncAdaptive || (level == AncAdaptive && !has("ancAdaptive")))
        level = AncHigh;
    setAncMode(level);
}

void Device::setEqPreset(int preset) {
    if (preset != 0 && m_values.value("mutuallyExclusive").toBool() && m_values.value("spatial").toInt() > 0) {
        setValue("spatial", 0);
        write(Cmd::SpatialWrite, spatialPayload(0), [this] { read(Cmd::SpatialRead); });
    }
    // Picking a preset is the Simple tab: the advanced curve goes off.
    if (m_values.value("advancedEq").toBool()) {
        setValue("advancedEq", false);
        write(Cmd::AdvancedEqModeWrite, le16Bytes(0), [this] { read(Cmd::AdvancedEqModeRead); });
    }
    setValue("eqPreset", preset);
    write(Cmd::EqModeWrite, le16Bytes(preset), [this] { read(Cmd::EqModeRead); });
}

void Device::setCustomEq(double bass, double mid, double treble) {
    auto clampGain = [](double v) { return std::clamp(std::round(v * 2.0) / 2.0, -6.0, 6.0); };
    m_customEq = {clampGain(bass), clampGain(mid), clampGain(treble)};
    setValue("customBass", m_customEq.bass);
    setValue("customMid", m_customEq.mid);
    setValue("customTreble", m_customEq.treble);
    if (m_values.value("eqPreset").toInt() != 5)
        setEqPreset(5);
    m_customEqDebounce.start();
}

void Device::sendCustomEq() {
    CustomEqShape shape;
    shape.lowFreq = m_model.customEq.lowFreq;
    shape.lowQ = m_model.customEq.lowQ;
    shape.peakFreq = m_model.customEq.peakFreq;
    shape.peakQ = m_model.customEq.peakQ;
    shape.highFreq = m_model.customEq.highFreq;
    shape.highQ = m_model.customEq.highQ;
    write(Cmd::CustomEqWrite, customEqPayload(m_customEq, shape), [this] { read(Cmd::CustomEqRead); });
}

void Device::setAdvancedEq(bool on) {
    if (on && has("spaceOrAdvance") && m_values.value("spatial").toInt() > 0) {
        setValue("spatial", 0);
        write(Cmd::SpatialWrite, spatialPayload(0), [this] { read(Cmd::SpatialRead); });
    }
    setValue("advancedEq", on);
    write(Cmd::AdvancedEqModeWrite, le16Bytes(on ? 1 : 0), [this] { read(Cmd::AdvancedEqModeRead); });
    if (on)
        m_advancedEqDebounce.start();
}

void Device::setAdvancedBand(int index, double frequency, double gain, double q) {
    if (index < 0 || index >= m_bands.size())
        return;
    EqBand &b = m_bands[index];
    b.frequency = std::clamp(std::round(frequency), 20.0, 20000.0);
    b.gain = std::clamp(std::round(gain * 2.0) / 2.0, -6.0, 6.0);
    b.q = std::clamp(std::round(q * 10.0) / 10.0, 0.1, 10.0);
    emit valuesChanged();
    if (!m_values.value("advancedEq").toBool())
        setAdvancedEq(true);
    m_advancedEqDebounce.start();
}

void Device::addAdvancedBand() {
    if (m_bands.size() >= 8)
        return;
    // A new point goes into the widest gap of the curve, flat, with the
    // official default Q.
    QVector<double> freqs{20.0};
    for (const EqBand &b : m_bands)
        freqs << b.frequency;
    freqs << 20000.0;
    std::sort(freqs.begin(), freqs.end());
    double best = 1000, widest = 0;
    for (int i = 1; i < freqs.size(); ++i) {
        const double ratio = freqs[i] / freqs[i - 1];
        if (ratio > widest) {
            widest = ratio;
            best = std::sqrt(freqs[i] * freqs[i - 1]);
        }
    }
    m_bands.append({Peak, 0.0, std::round(best), 1.4});
    std::sort(m_bands.begin(), m_bands.end(), [](const EqBand &a, const EqBand &b) { return a.frequency < b.frequency; });
    emit valuesChanged();
    m_advancedEqDebounce.start();
}

void Device::removeAdvancedBand(int index) {
    if (index < 0 || index >= m_bands.size() || m_bands.size() <= 1)
        return;
    m_bands.removeAt(index);
    emit valuesChanged();
    m_advancedEqDebounce.start();
}

void Device::setAdvancedBands(const QVariantList &bands) {
    QVector<EqBand> out;
    for (const QVariant &v : bands) {
        const QVariantMap m = v.toMap();
        out.append({Peak, std::clamp(m.value("gain").toDouble(), -6.0, 6.0),
                    std::clamp(m.value("frequency").toDouble(), 20.0, 20000.0),
                    std::clamp(m.value("q").toDouble(), 0.1, 10.0)});
        if (out.size() == 8)
            break;
    }
    if (out.isEmpty())
        return;
    m_bands = out;
    emit valuesChanged();
    if (!m_values.value("advancedEq").toBool())
        setAdvancedEq(true);
    m_advancedEqDebounce.start();
}

void Device::resetAdvancedEq() {
    m_bands = defaultAdvancedBands();
    emit valuesChanged();
    m_advancedEqDebounce.start();
}

void Device::sendAdvancedEq() {
    write(Cmd::AdvancedEqWrite, advancedEqPayload(m_bands, m_model.advancedTotalGain),
          [this] { read(Cmd::AdvancedEqRead, le16Bytes(0)); });
}

void Device::setBass(bool on, int level) {
    level = std::clamp(level, 1, 5);
    if (on && has("spaceOrAdvance") && m_values.value("spatial").toInt() > 0) {
        setValue("spatial", 0);
        write(Cmd::SpatialWrite, spatialPayload(0), [this] { read(Cmd::SpatialRead); });
    }
    setValue("bassOn", on);
    setValue("bassLevel", level);
    write(Cmd::BassWrite, bassPayload(on, level), [this] { read(Cmd::BassRead); });
}

void Device::setSpatial(int mode) {
    if (mode > 0) {
        if (m_values.value("ldac").toBool()) {
            setValue("ldac", false);
            write(Cmd::LdacWrite, le16Bytes(0), [this] { read(Cmd::LdacRead); });
        }
        if (m_ancMode == AncAdaptive)
            setAncMode(AncHigh);
        if (has("spaceOrAdvance")) {
            if (m_values.value("advancedEq").toBool()) {
                setValue("advancedEq", false);
                write(Cmd::AdvancedEqModeWrite, le16Bytes(0), [this] { read(Cmd::AdvancedEqModeRead); });
            }
            if (m_values.value("bassOn").toBool()) {
                const int level = m_values.value("bassLevel", 1).toInt();
                setValue("bassOn", false);
                write(Cmd::BassWrite, bassPayload(false, level), [this] { read(Cmd::BassRead); });
            }
        }
        if (m_values.value("mutuallyExclusive").toBool() && m_values.value("eqPreset").toInt() != 0) {
            setValue("eqPreset", 0);
            write(Cmd::EqModeWrite, le16Bytes(0), [this] { read(Cmd::EqModeRead); });
        }
    }
    setValue("spatial", mode);
    write(Cmd::SpatialWrite, spatialPayload(mode), [this] { read(Cmd::SpatialRead); });
}

void Device::setLdac(bool on) {
    if (on) {
        if (m_values.value("spatial").toInt() > 0) {
            setValue("spatial", 0);
            write(Cmd::SpatialWrite, spatialPayload(0), [this] { read(Cmd::SpatialRead); });
        }
        if (m_ancMode == AncAdaptive)
            setAncMode(AncHigh);
        if (m_values.value("personalSound").toBool()) {
            setValue("personalSound", false);
            write(Cmd::PersonalSoundWrite, QByteArray(1, 0), [this] { read(Cmd::PersonalSoundRead); });
        }
    }
    setValue("ldac", on);
    // The Ear (3) takes LDAC as 2 (1 would be LHDC).
    write(Cmd::LdacWrite, le16Bytes(on ? 2 : 0), [this] { read(Cmd::LdacRead); });
}

void Device::setLowLag(bool on) {
    setValue("lowLag", on);
    write(Cmd::LowLagWrite, le16Bytes(on ? 1 : 2), [this] { read(Cmd::LowLagRead); });
}

void Device::setInEarDetection(bool on) {
    setValue("inEar", on);
    write(Cmd::InEarWrite, inEarPayload(on), [this] { read(Cmd::InEarRead); });
}

void Device::setPersonalSound(bool on) {
    if (on && m_values.value("ldac").toBool()) {
        setValue("ldac", false);
        write(Cmd::LdacWrite, le16Bytes(0), [this] { read(Cmd::LdacRead); });
    }
    setValue("personalSound", on);
    write(Cmd::PersonalSoundWrite, QByteArray(1, char(on ? 1 : 0)), [this] { read(Cmd::PersonalSoundRead); });
}

void Device::setDualConnection(bool on) {
    setValue("dual", on);
    write(Cmd::DualWrite, QByteArray(1, char(on ? 1 : 0)), [this] { read(Cmd::DualRead); });
    QTimer::singleShot(1500, this, &Device::refreshPairedDevices);
}

void Device::connectPairedDevice(const QString &address, bool connect) {
    write(Cmd::DualConnectWrite, dualConnectPayload(address, connect), [this] { refreshPairedDevices(); });
    QTimer::singleShot(2500, this, &Device::refreshPairedDevices);
}

void Device::refreshPairedDevices() {
    if (m_state == Connected && has("dual"))
        read(Cmd::DualListRead, le16Bytes(0));
}

void Device::setSuperMic(bool on) {
    setValue("superMic", on);
    write(Cmd::SuperMicWrite, QByteArray(1, char(on ? 1 : 0)), [this] { read(Cmd::SuperMicRead); });
}

void Device::setCallTransparency(bool on) {
    setValue("callTransparency", on);
    write(Cmd::CallTransparencyWrite, QByteArray(1, char(on ? 1 : 0)), [this] { read(Cmd::CallTransparencyRead); });
}

void Device::setWalkieTalkie(bool on) {
    // Turning walkie-talkie mode off turns Super Mic off with it.
    if (!on && m_values.value("superMic").toBool())
        setValue("superMic", false);
    setValue("walkieTalkie", on);
    write(Cmd::WalkieTalkieWrite, QByteArray(1, char(on ? 1 : 0)), [this] {
        read(Cmd::WalkieTalkieRead);
        read(Cmd::SuperMicRead);
    });
}

void Device::setGesture(int device, int gesture, int action) {
    QList<Gesture> changed{{device, 1, gesture, action}};
    // A noise-control cycle stays the same on both earbuds, as the official
    // Controls page keeps it.
    const bool cycle = action == 10 || (action >= 20 && action <= 22);
    if (cycle && (device == PartLeft || device == PartRight)) {
        const int other = device == PartLeft ? PartRight : PartLeft;
        const int current = gestureAction(other, gesture);
        if (current == 10 || (current >= 20 && current <= 22))
            changed.append({other, 1, gesture, action});
    }
    for (const Gesture &c : changed) {
        bool found = false;
        for (Gesture &g : m_gestures)
            if (g.device == c.device && g.gesture == c.gesture) {
                g.action = c.action;
                found = true;
            }
        if (!found)
            m_gestures.append(c);
    }
    emit valuesChanged();
    write(Cmd::GesturesWrite, gesturesPayload(changed), [this] { read(Cmd::GesturesRead); });
}

void Device::resetGestures() {
    const QList<Gesture> defaults = parseGestures(QByteArray::fromHex(m_model.defaultControl.toLatin1()));
    if (defaults.isEmpty())
        return;
    m_gestures = defaults;
    emit valuesChanged();
    write(Cmd::GesturesWrite, gesturesPayload(defaults), [this] { read(Cmd::GesturesRead); });
}

void Device::startFitTest() {
    setValue("fitLeft", -1);
    setValue("fitRight", -1);
    setValue("fitRunning", true);
    // The result arrives as a notification; the official test lasts
    // testFitDuration seconds, so allow a generous margin.
    m_fitTimeout.setInterval((m_model.features.value("fitDuration", 10).toInt() + 20) * 1000);
    m_fitTimeout.start();
    write(Cmd::FitTestWrite, QByteArray(1, 1), [this] {
        m_fitTimeout.stop();
        setValue("fitRunning", false);
    });
}

void Device::ring(int side, bool on) {
    const QString key = side == PartLeft ? "ringLeft" : "ringRight";
    QTimer &timer = side == PartLeft ? m_ringLeft : m_ringRight;
    setValue(key, on);
    if (on)
        timer.start();
    else
        timer.stop();
    write(Cmd::FindWrite, findPayload(side, on), [this, key] { setValue(key, false); });
}

void Device::stopRing(int side) {
    if (m_values.value(side == PartLeft ? "ringLeft" : "ringRight").toBool())
        ring(side, false);
}
