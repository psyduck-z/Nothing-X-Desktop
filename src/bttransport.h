#pragma once

#include "transport.h"

#include <QBluetoothSocket>
#include <QBluetoothUuid>
#include <QPointer>

// RFCOMM to the earbuds through Qt Bluetooth: BlueZ on Linux, WinRT on
// Windows. Connects the way the official app does
// (XBaseSppConnector.connectSocket): a secure socket to the channel SDP
// reports for Nothing's control service, tried twice, and only then the
// fixed channel 15 as a last resort.
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
    static const int MaxAttempts;

private:
    void attempt();
    QPointer<QBluetoothSocket> m_socket;
    QString m_address;
    int m_attempt = 0;
    bool m_closing = false;
};
