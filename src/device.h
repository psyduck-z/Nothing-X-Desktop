#pragma once

#include "modelconfig.h"
#include "protocol.h"

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <functional>
#include <optional>

class OfficialPack;
class Settings;
class Transport;

// The connected earbuds: their state, and every setting the official Nothing
// X app can change. Commands go out one at a time and every write's reply
// status is checked (the earbuds answer each write with a status code); a
// rejected write is undone in the UI and the value is read back.
class Device : public QObject {
    Q_OBJECT
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString address READ address NOTIFY stateChanged)
    Q_PROPERTY(QString bluetoothName READ bluetoothName NOTIFY stateChanged)
    Q_PROPERTY(QString modelId READ modelId NOTIFY modelChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY modelChanged)
    Q_PROPERTY(QString colorId READ colorId NOTIFY modelChanged)
    Q_PROPERTY(QVariantMap features READ features NOTIFY modelChanged)
    Q_PROPERTY(QVariantList eqPresets READ eqPresets NOTIFY modelChanged)
    Q_PROPERTY(QVariantList gestureGroups READ gestureGroups NOTIFY modelChanged)
    Q_PROPERTY(int batteryLeft READ batteryLeft NOTIFY batteryChanged)
    Q_PROPERTY(int batteryRight READ batteryRight NOTIFY batteryChanged)
    Q_PROPERTY(int batteryCase READ batteryCase NOTIFY batteryChanged)
    Q_PROPERTY(bool chargingLeft READ chargingLeft NOTIFY batteryChanged)
    Q_PROPERTY(bool chargingRight READ chargingRight NOTIFY batteryChanged)
    Q_PROPERTY(bool chargingCase READ chargingCase NOTIFY batteryChanged)
    Q_PROPERTY(bool leftInEar READ leftInEar NOTIFY batteryChanged)
    Q_PROPERTY(bool rightInEar READ rightInEar NOTIFY batteryChanged)
    Q_PROPERTY(int ancMode READ ancMode NOTIFY ancChanged)
    Q_PROPERTY(int ancLevel READ ancLevel NOTIFY ancChanged)
    // Everything else, keyed by name: QML reads device.values.lowLag.
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList advancedBands READ advancedBands NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList gestures READ gestures NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList pairedDevices READ pairedDevices NOTIFY valuesChanged)

public:
    enum State { Disconnected, Connecting, Connected };
    Q_ENUM(State)

    Device(Transport *transport, Settings *settings, OfficialPack *pack, QObject *parent = nullptr);
    ~Device() override;

    int state() const { return m_state; }
    QString error() const { return m_error; }
    QString address() const { return m_address; }
    QString bluetoothName() const { return m_bluetoothName; }
    QString modelId() const { return m_model.id; }
    QString displayName() const;
    QString colorId() const { return m_colorId; }
    QVariantMap features() const { return m_model.features; }
    QVariantList eqPresets() const { return m_model.eqPresets; }
    QVariantList gestureGroups() const { return m_model.gestureGroups; }
    int batteryLeft() const { return m_battery.left.level; }
    int batteryRight() const { return m_battery.right.level; }
    int batteryCase() const { return m_battery.caseUnit.level; }
    bool chargingLeft() const { return m_battery.left.charging; }
    bool chargingRight() const { return m_battery.right.charging; }
    bool chargingCase() const { return m_battery.caseUnit.charging; }
    bool leftInEar() const { return Proto::Wear::inEar(m_wear.left); }
    bool rightInEar() const { return Proto::Wear::inEar(m_wear.right); }
    int ancMode() const { return m_ancMode; }
    int ancLevel() const { return m_ancLevel; }
    QVariantMap values() const { return m_values; }
    QVariantList advancedBands() const;
    QVariantList gestures() const;
    QVariantList pairedDevices() const;
    const ModelConfig &model() const { return m_model; }

    Q_INVOKABLE bool has(const QString &feature) const { return m_model.has(feature); }
    Q_INVOKABLE int gestureAction(int device, int gesture) const;
    // The official app's warning before a change that turns something else
    // off (spatial audio vs LDAC, and so on); empty when there is none.
    Q_INVOKABLE QString conflict(const QString &setting, const QVariant &value) const;

public slots:
    void connectToDevice(const QString &address, const QString &name);
    void connectToLast();
    void disconnectFromDevice();
    void refresh();

    // Modes as the earbuds number them: 1 High, 2 Mid, 3 Low, 4 Adaptive,
    // 5 Off, 7 Transparency.
    void setAncMode(int mode);
    // "Noise cancellation" restores the last level used.
    void setNoiseCancellation();
    // 0 Balanced, 1 Voice, 2 More treble, 3 More bass, 5 Custom.
    void setEqPreset(int preset);
    void setCustomEq(double bass, double mid, double treble);
    void setAdvancedEq(bool on);
    void setAdvancedBand(int index, double frequency, double gain, double q);
    void addAdvancedBand();
    void removeAdvancedBand(int index);
    void setAdvancedBands(const QVariantList &bands);
    void resetAdvancedEq();
    void setBass(bool on, int level);
    // 0 off, 1 fixed, 2 head tracking.
    void setSpatial(int mode);
    void setLdac(bool on);
    void setLowLag(bool on);
    void setInEarDetection(bool on);
    void setPersonalSound(bool on);
    void setDualConnection(bool on);
    void connectPairedDevice(const QString &address, bool connect);
    void refreshPairedDevices();
    void setSuperMic(bool on);
    void setCallTransparency(bool on);
    void setWalkieTalkie(bool on);
    // device 2 left / 3 right / 4 case; gesture 2 double, 3 triple, 7 hold,
    // 9 double-hold; action as the official app numbers it.
    void setGesture(int device, int gesture, int action);
    void resetGestures();
    void startFitTest();
    // side 2 left / 3 right.
    void ring(int side, bool on);

signals:
    void stateChanged();
    void modelChanged();
    void batteryChanged();
    void ancChanged();
    void valuesChanged();
    // A short message for the user (a change the earbuds refused, say).
    void notice(const QString &message);
    void fitTestFinished(int left, int right);

private:
    using Handler = std::function<void(const Proto::Frame &)>;
    struct Request {
        quint16 cmd;
        QByteArray payload;
        Handler onReply;
        int retries = 0;
    };
    void setState(State state, const QString &error = {});
    void setValue(const QString &key, const QVariant &value);
    void enqueue(Request request);
    void read(quint16 cmd, const QByteArray &payload = {});
    // Sends a write; when the earbuds reject it, `undo` re-reads the setting.
    void write(quint16 cmd, const QByteArray &payload, const std::function<void()> &undo = {});
    void pump();
    void onBytes(const QByteArray &bytes);
    void handle(const Proto::Frame &frame);
    void applyModel(const QString &id);
    void readAll();
    void sendCustomEq();
    void sendAdvancedEq();
    void stopRing(int side);
    static QString statusText(int status);

    Transport *m_transport;
    Settings *m_settings;
    OfficialPack *m_pack;
    State m_state = Disconnected;
    QString m_error;
    QString m_address, m_bluetoothName;
    ModelConfig m_model;
    QString m_colorId = QStringLiteral("01");

    Proto::Decoder m_decoder;
    Proto::FsnCounter m_fsn;
    QQueue<Request> m_queue;
    std::optional<Request> m_inFlight;
    QTimer m_timeout, m_flush, m_batteryPoll, m_customEqDebounce, m_advancedEqDebounce;
    QTimer m_ringLeft, m_ringRight, m_fitTimeout;

    Proto::Battery m_battery;
    Proto::Wear m_wear;
    int m_ancMode = Proto::AncOff;
    int m_ancLevel = Proto::AncHigh;
    QVariantMap m_values;
    Proto::CustomEq m_customEq;
    QVector<Proto::EqBand> m_bands;
    QList<Proto::Gesture> m_gestures;
    QList<Proto::PairedDevice> m_paired;
};
