#include "bttransport.h"

#include <QBluetoothAddress>

BluetoothTransport::BluetoothTransport(QObject *parent) : Transport(parent) {}

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

void BluetoothTransport::attempt(bool byChannel) {
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
    }
    m_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);
    connect(m_socket, &QBluetoothSocket::connected, this, &Transport::opened);
    connect(m_socket, &QBluetoothSocket::disconnected, this, [this] {
        if (!m_closing)
            emit closed();
    });
    connect(m_socket, &QBluetoothSocket::readyRead, this, [this] { emit received(m_socket->readAll()); });
    connect(m_socket, &QBluetoothSocket::errorOccurred, this, [this](QBluetoothSocket::SocketError error) {
        if (m_closing)
            return;
        // The service lookup failed or the connection was refused before it
        // opened: try the known channel once before giving up.
        if (!m_triedChannel && m_socket->state() != QBluetoothSocket::SocketState::ConnectedState &&
            error != QBluetoothSocket::SocketError::RemoteHostClosedError) {
            m_triedChannel = true;
            QMetaObject::invokeMethod(this, [this] { attempt(true); }, Qt::QueuedConnection);
            return;
        }
        emit failed(m_socket->errorString().isEmpty() ? tr("Could not reach the earbuds") : m_socket->errorString());
    });

    const QBluetoothAddress address(m_address);
    if (byChannel)
        m_socket->connectToService(address, FallbackChannel);
    else
        m_socket->connectToService(address, serviceUuid());
}

void BluetoothTransport::open(const QString &address) {
    m_address = address;
    m_triedChannel = false;
    m_closing = false;
    attempt(false);
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
