#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

// A byte pipe to the earbuds. The real one is an RFCOMM socket
// (BluetoothTransport); the demo one simulates a pair of earbuds so the UI
// can be exercised, and tested, without hardware.
class Transport : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~Transport() override = default;

    virtual void open(const QString &address) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void write(const QByteArray &bytes) = 0;

signals:
    void opened();
    void closed();
    void received(const QByteArray &bytes);
    void failed(const QString &message);
};
