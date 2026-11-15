#include "bttransport.h"

#include <QBluetoothAddress>

BluetoothTransport::BluetoothTransport(QObject *parent) : Transport(parent) {}

// BlueZ's D-Bus sockets (what Qt uses on Linux) can only connect by service
// UUID, so the fixed-channel fallback exists on Windows only.
#ifdef Q_OS_LINUX
const int BluetoothTransport::MaxAttempts = 2;
#else
const int BluetoothTransport::MaxAttempts = 3;
#endif

BluetoothTransport::~BluetoothTransport() {
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
    }
}

QBluetoothUuid BluetoothTransport::serviceUuid() {
    // Nothing's control service, as used by the Nothing X app and the
    // community projects credited in the README.
    return QBluetoothUuid(QStringLiteral("aeac4a03-dff5-498f-843a-34487cf133eb"));
}

void BluetoothTransport::attempt() {
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
    }
    m_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    // Android's createRfcommSocketToServiceRecord: authenticated and encrypted.
    m_socket->setPreferredSecurityFlags(QBluetooth::Security::Authentication | QBluetooth::Security::Encryption |
                                        QBluetooth::Security::Secure);
    connect(m_socket, &QBluetoothSocket::connected, this, &Transport::opened);
    connect(m_socket, &QBluetoothSocket::disconnected, this, [this] {
        if (!m_closing)
            emit closed();
    });
    connect(m_socket, &QBluetoothSocket::readyRead, this, [this] { emit received(m_socket->readAll()); });
    connect(m_socket, &QBluetoothSocket::errorOccurred, this, [this](QBluetoothSocket::SocketError error) {
        if (m_closing)
            return;
        // The lookup or the connection failed before it opened: the UUID
        // once more, then channel 15, like the official app.
        if (m_attempt + 1 < MaxAttempts && m_socket->state() != QBluetoothSocket::SocketState::ConnectedState &&
            error != QBluetoothSocket::SocketError::RemoteHostClosedError) {
            ++m_attempt;
            QMetaObject::invokeMethod(this, [this] { attempt(); }, Qt::QueuedConnection);
            return;
        }
        emit failed(tr("Couldn't reach the earbuds. Take them out of the case; if the Nothing X phone app is open, close it for a moment."));
    });

    const QBluetoothAddress address(m_address);
    if (m_attempt >= 2)
        m_socket->connectToService(address, FallbackChannel);
    else
        m_socket->connectToService(address, serviceUuid());
}

void BluetoothTransport::open(const QString &address) {
    m_address = address;
    m_attempt = 0;
    m_closing = false;
    attempt();
}

void BluetoothTransport::close() {
    m_closing = true;
    if (m_socket && m_socket->state() != QBluetoothSocket::SocketState::UnconnectedState)
        m_socket->disconnectFromService();
}

bool BluetoothTransport::isOpen() const {
    return m_socket && m_socket->state() == QBluetoothSocket::SocketState::ConnectedState;
}

void BluetoothTransport::write(const QByteArray &bytes) {
    if (isOpen())
        m_socket->write(bytes);
}
