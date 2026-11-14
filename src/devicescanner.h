#pragma once

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothLocalDevice>
#include <QObject>
#include <QTimer>
#include <QVariantList>

// Lists the Nothing / CMF audio devices this computer is paired with, and
// which of them are connected right now. The earbuds must be paired in the
// OS first (Omarchy: Bluetooth in the bar or `bluetui`; Windows: Settings >
// Bluetooth & devices); the app then talks to them over RFCOMM.
class DeviceScanner : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList devices READ devices NOTIFY devicesChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool adapterAvailable READ adapterAvailable NOTIFY adapterChanged)
    Q_PROPERTY(bool adapterPoweredOn READ adapterPoweredOn NOTIFY adapterChanged)
    Q_PROPERTY(QString error READ error NOTIFY scanningChanged)

public:
    explicit DeviceScanner(QObject *parent = nullptr);

    QVariantList devices() const { return m_devices; }
    bool scanning() const;
    bool adapterAvailable() const;
    bool adapterPoweredOn() const;
    QString error() const { return m_error; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void powerOnAdapter();
    Q_INVOKABLE bool isConnected(const QString &address) const;

    static bool looksLikeNothing(const QString &name);

signals:
    void devicesChanged();
    void scanningChanged();
    void adapterChanged();
    // A device we know about just connected at the OS level.
    void deviceConnected(const QString &address, const QString &name);

private:
    void add(const QBluetoothDeviceInfo &info);
    void updateConnected();

    QBluetoothLocalDevice m_local;
    QBluetoothDeviceDiscoveryAgent *m_agent = nullptr;
    QVariantList m_devices;
    QString m_error;
    QTimer m_stop;
};
