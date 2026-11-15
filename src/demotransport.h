#pragma once

#include "protocol.h"
#include "transport.h"

#include <QMap>
#include <QTimer>

// A simulated Nothing Ear (3): it answers every command the app sends with
// the payload layouts the official app parses, keeps the settings it is
// given, and pushes battery, noise-control and fit-test notifications like
// the real earbuds. Used by `nothingx --demo`, the screenshots and the tests.
class DemoTransport : public Transport {
    Q_OBJECT
public:
    explicit DemoTransport(QObject *parent = nullptr);

    void open(const QString &address) override;
    void close() override;
    bool isOpen() const override { return m_open; }
    void write(const QByteArray &bytes) override;

    // Delay before replies; 0 in tests.
    void setLatency(int ms) { m_latency = ms; }
    // Fit test length; shorter in tests.
    void setFitDuration(int ms) { m_fitDuration = ms; }
    // Status code for the next write's reply (to test rejected writes).
    void rejectNextWrite(int status) { m_rejectStatus = status; }
    // Every frame the app has sent, for tests.
    QList<Proto::Frame> sent() const { return m_sent; }
    QByteArray state(quint16 readCommand) const { return m_state.value(readCommand); }
    void push(quint16 cmd, const QByteArray &payload);

private:
    void send(quint16 cmd, int status, quint8 fsn, const QByteArray &payload);
    void handle(const Proto::Frame &frame);
    QByteArray batteryPayload() const;
    QByteArray dualListPayload() const;

    bool m_open = false;
    int m_latency = 25;
    int m_fitDuration = 6000;
    int m_rejectStatus = 0;
    Proto::Decoder m_decoder;
    QList<Proto::Frame> m_sent;
    QMap<quint16, QByteArray> m_state; // read command -> current reply payload
    int m_left = 86, m_right = 91, m_case = 64;
    bool m_caseCharging = true;
    struct Host {
        QByteArray mac;
        QByteArray name;
        bool connected;
        bool thisHost;
    };
    QList<Host> m_hosts;
    QTimer m_drain;
};
