#pragma once

#include "transport.h"

#include <QBluetoothSocket>
#include <QBluetoothUuid>
#include <QPointer>

// RFCOMM to the earbuds through Qt Bluetooth: BlueZ on Linux, WinRT on
// Windows. The channel is found by SDP from the service UUID, so the same
// code works on both. Some Ear (3) firmware leaves the UUID out of its SDP
// records; then the known channel is tried directly.
class BluetoothTransport : public Transport {
    Q_OBJECT
public:
    explicit BluetoothTransport(QObject *parent = nullptr);
    ~BluetoothTransport() override;

    void open(const QString &address) override;
    void close() override;
    bool isOpen() const override;
    void write(const QByteArray &bytes) override;

    static QBluetoothUuid serviceUuid();
    static constexpr quint16 FallbackChannel = 15;

private:
    void attempt(bool byChannel);
    QPointer<QBluetoothSocket> m_socket;
    QString m_address;
    bool m_triedChannel = false;
    bool m_closing = false;
};
