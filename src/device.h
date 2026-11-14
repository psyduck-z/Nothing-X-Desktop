#pragma once

#include "models.h"
#include "protocol.h"

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class Settings;
class Transport;

// The connected earbuds: their state, and every setting the Nothing X app
// can change. Commands go out one at a time (the firmware expects a request,
// then its reply); writes update the UI optimistically and are then read back,
// because the firmware acknowledges writes it chooses to ignore.
class Device : public QObject {
    Q_OBJECT
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY stateChanged)
    Q_PROPERTY(QString address READ address NOTIFY stateChanged)
    Q_PROPERTY(QString bluetoothName READ bluetoothName NOTIFY stateChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY modelChanged)
    Q_PROPERTY(QString modelId READ modelId NOTIFY modelChanged)
    Q_PROPERTY(QStringList features READ features NOTIFY modelChanged)
    Q_PROPERTY(bool ancLevels READ ancLevels NOTIFY modelChanged)
    Q_PROPERTY(bool ancAdaptive READ ancAdaptive NOTIFY modelChanged)
    Q_PROPERTY(bool hasCase READ hasCase NOTIFY modelChanged)
    Q_PROPERTY(int batteryLeft READ batteryLeft NOTIFY batteryChanged)
    Q_PROPERTY(int batteryRight READ batteryRight NOTIFY batteryChanged)
    Q_PROPERTY(int batteryCase READ batteryCase NOTIFY batteryChanged)
    Q_PROPERTY(bool chargingLeft READ chargingLeft NOTIFY batteryChanged)
    Q_PROPERTY(bool chargingRight READ chargingRight NOTIFY batteryChanged)
    Q_PROPERTY(bool chargingCase READ chargingCase NOTIFY batteryChanged)
    Q_PROPERTY(int ancMode READ ancMode NOTIFY ancChanged)
    // Everything else, keyed by feature: QML reads device.values.lowLatency.
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList pairedDevices READ pairedDevices NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList gestures READ gestures NOTIFY valuesChanged)
    Q_PROPERTY(QVariantList advancedBands READ advancedBands NOTIFY valuesChanged)

public:
    enum State { Disconnected, Connecting, Connected };
    Q_ENUM(State)
    // Noise control as the UI shows it.
    enum AncMode { AncOff, AncTransparency, AncAdaptive, AncHigh, AncMid, AncLow };
    Q_ENUM(AncMode)

    Device(Transport *transport, Settings *settings, QObject *parent = nullptr);
    ~Device() override;

    int state() const { return m_state; }
    QString error() const { return m_error; }
    QString address() const { return m_address; }
    QString bluetoothName() const { return m_bluetoothName; }
    QString displayName() const { return m_model.name; }
    QString modelId() const { return m_model.id; }
    QStringList features() const;
    bool ancLevels() const { return m_model.ancLevels; }
    bool ancAdaptive() const { return m_model.ancAdaptive; }
    bool hasCase() const { return m_model.hasCase; }
    int batteryLeft() const { return m_battery.left.level >= 0 ? m_battery.left.level : m_battery.single.level; }
    int batteryRight() const { return m_battery.right.level; }
    int batteryCase() const { return m_battery.caseUnit.level; }
    bool chargingLeft() const { return m_battery.left.charging; }
    bool chargingRight() const { return m_battery.right.charging; }
    bool chargingCase() const { return m_battery.caseUnit.charging; }
    int ancMode() const { return m_ancMode; }
    QVariantMap values() const { return m_values; }
    QVariantList pairedDevices() const;
    QVariantList gestures() const;
    QVariantList advancedBands() const;

    Q_INVOKABLE bool hasFeature(const QString &feature) const { return m_model.has(feature); }
    static bool isNoiseCancelling(int mode) { return mode == AncAdaptive || mode == AncHigh || mode == AncMid || mode == AncLow; }
    int lastNoiseCancellingMode() const { return m_lastNcMode; }
    static quint8 ancToByte(int mode);
    static int ancFromByte(quint8 byte);

public slots:
    void connectToDevice(const QString &address, const QString &name);
    void connectToLast();
    void disconnectFromDevice();
    void refresh();

    void setAncMode(int mode);
    // 0 Balanced, 1 More voice, 2 More treble, 3 More bass, 5 Custom.
    void setEqPreset(int preset);
    void setAdvancedEq(bool on);
    void setCustomEq(double bass, double mid, double treble);
    void setAdvancedBandGain(int index, double gain);
    void resetAdvancedEq();
    void setBass(bool on, int level);
    void setSpatial(int mode);
    void setPersonalSound(bool on);
    void setInEarDetection(bool on);
    void setLowLatency(bool on);
    // 0 AAC (stable connection), 2 LDAC (high quality). The earbuds restart.
    void setCodec(int codec);
    void setDualConnection(bool on);
    void connectPairedDevice(const QString &address, bool connect);
    void refreshPairedDevices();
    void setSuperMic(bool on);
    void setMicMode(int mode);
    void setCallTransparency(bool on);
    void setWalkieTalkie(bool on);
    // device 2 left / 3 right / 4 case; type 2 double, 3 triple, 7 hold, 9 double-hold.
    void setGesture(int device, int type, int action);
    void startFitTest();
    // side 2 left / 3 right.
    void ring(int side, bool on);

public:
    Q_INVOKABLE int gestureAction(int device, int type) const;

signals:
    void stateChanged();
    void modelChanged();
    void batteryChanged();
    void ancChanged();
    void valuesChanged();
    void fitTestFinished(int left, int right);

private:
    struct Request {
        quint16 cmd;
        QByteArray payload;
    };
    void setState(State state, const QString &error = {});
    void setValue(const QString &key, const QVariant &value);
    void enqueue(quint16 cmd, const QByteArray &payload = {});
    void read(quint16 cmd, const QByteArray &payload = {}) { enqueue(cmd, payload); }
    void write(quint16 cmd, const QByteArray &payload, quint16 readBack = 0);
    void pump();
    void onBytes(const QByteArray &bytes);
    void handle(const Proto::Frame &frame);
    void applyModel(const Model &model);
    void readFeatures();
    void sendCustomEq();
    void sendAdvancedEq();

    Transport *m_transport;
    Settings *m_settings;
    State m_state = Disconnected;
    QString m_error;
    QString m_address, m_bluetoothName;
    Model m_model;

    Proto::Decoder m_decoder;
    QQueue<Request> m_queue;
    std::optional<Request> m_inFlight;
    quint8 m_fsn = 0;
    QTimer m_timeout;
    QTimer m_batteryPoll;
    QTimer m_customEqDebounce, m_advancedEqDebounce;
    bool m_featuresRead = false;

    Proto::Battery m_battery;
    int m_ancMode = AncOff;
    int m_lastNcMode = AncHigh;
    QVariantMap m_values;
    QList<Proto::Gesture> m_gestures;
    QList<Proto::PairedDevice> m_paired;
    QVector<Proto::EqBand> m_bands;
    Proto::CustomEq m_customEq;
};
