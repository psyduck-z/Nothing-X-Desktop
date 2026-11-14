#pragma once

#include "protocol.h"
#include "transport.h"

#include <QMap>
#include <QTimer>

// A simulated Nothing Ear (3): it answers every command the app sends the way
// the real earbuds do (payload layouts from real captures), keeps the
// settings it is given, and pushes battery and fit-test events. Used by
// `nothingx --demo` and by the tests.
class DemoTransport : public Transport {
    Q_OBJECT
public:
    explicit DemoTransport(QObject *parent = nullptr);

    void open(const QString &address) override;
    void close() override;
    bool isOpen() const override { return m_open; }
    void write(const QByteArray &bytes) override;

    // Delay before replies, 0 in tests.
    void setLatency(int ms) { m_latency = ms; }
    // Every frame the app has sent, for tests.
    QList<Proto::Frame> sent() const { return m_sent; }
    QByteArray state(quint16 readCommand) const { return m_state.value(readCommand); }

private:
    void reply(const Proto::Frame &request, const QByteArray &payload);
    void push(quint16 cmd, const QByteArray &payload);
    void handle(const Proto::Frame &frame);
    QByteArray batteryPayload() const;

    bool m_open = false;
    int m_latency = 25;
    Proto::Decoder m_decoder;
    QList<Proto::Frame> m_sent;
    QMap<quint16, QByteArray> m_state; // read command -> current reply payload
    int m_left = 86, m_right = 91, m_case = 64;
    QTimer m_drain;
};
