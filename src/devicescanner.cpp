#include "devicescanner.h"

DeviceScanner::DeviceScanner(QObject *parent) : QObject(parent) {
    if (!m_local.isValid())
        return;
    m_agent = new QBluetoothDeviceDiscoveryAgent(m_local.address(), this);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &DeviceScanner::add);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::deviceUpdated, this,
            [this](const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields) { add(info); });
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::finished, this, &DeviceScanner::scanningChanged);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &DeviceScanner::scanningChanged);
    connect(m_agent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, [this] {
        m_error = m_agent->errorString();
        emit scanningChanged();
    });
    connect(&m_local, &QBluetoothLocalDevice::hostModeStateChanged, this, &DeviceScanner::adapterChanged);
    connect(&m_local, &QBluetoothLocalDevice::deviceConnected, this, [this](const QBluetoothAddress &a) {
        updateConnected();
        for (const auto &d : std::as_const(m_devices)) {
            const auto m = d.toMap();
            if (m.value("address").toString() == a.toString())
                emit deviceConnected(a.toString(), m.value("name").toString());
        }
    });
    connect(&m_local, &QBluetoothLocalDevice::deviceDisconnected, this, &DeviceScanner::updateConnected);

    // Known devices are reported within the first moments of a scan; there is
    // no need to keep the radio busy discovering strangers for long.
    m_stop.setSingleShot(true);
    m_stop.setInterval(6000);
    connect(&m_stop, &QTimer::timeout, this, [this] {
        if (m_agent->isActive())
            m_agent->stop();
    });
}

bool DeviceScanner::looksLikeNothing(const QString &name) {
    const QString n = name.toLower();
    return n.contains("nothing") || n.startsWith("ear (") || n.startsWith("ear(") || n.startsWith("cmf") ||
           n.contains("headphone (");
}

bool DeviceScanner::scanning() const { return m_agent && m_agent->isActive(); }
bool DeviceScanner::adapterAvailable() const { return m_local.isValid(); }
bool DeviceScanner::adapterPoweredOn() const {
    return m_local.isValid() && m_local.hostMode() != QBluetoothLocalDevice::HostPoweredOff;
}

void DeviceScanner::powerOnAdapter() {
    if (m_local.isValid())
        m_local.powerOn();
}

bool DeviceScanner::isConnected(const QString &address) const {
    return m_local.isValid() && m_local.connectedDevices().contains(QBluetoothAddress(address));
}

void DeviceScanner::refresh() {
    m_error.clear();
    if (!m_agent) {
        m_error = tr("No Bluetooth adapter found");
        emit scanningChanged();
        return;
    }
    if (!m_agent->isActive())
        m_agent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
    m_stop.start();
    emit scanningChanged();
    updateConnected();
}

void DeviceScanner::add(const QBluetoothDeviceInfo &info) {
    if (!looksLikeNothing(info.name()))
        return;
    const QString address = info.address().toString();
    QVariantMap entry{{"name", info.name()},
                      {"address", address},
                      {"connected", isConnected(address)},
                      {"paired", m_local.pairingStatus(info.address()) != QBluetoothLocalDevice::Unpaired}};
    for (int i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].toMap().value("address") == address) {
            if (m_devices[i].toMap() != entry) {
                m_devices[i] = entry;
                emit devicesChanged();
            }
            return;
        }
    }
    m_devices.append(entry);
    emit devicesChanged();
}

void DeviceScanner::updateConnected() {
    bool dirty = false;
    for (auto &d : m_devices) {
        auto m = d.toMap();
        const bool c = isConnected(m.value("address").toString());
        if (m.value("connected").toBool() != c) {
            m["connected"] = c;
            d = m;
            dirty = true;
        }
    }
    if (dirty)
        emit devicesChanged();
}
