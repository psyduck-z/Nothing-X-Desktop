#include "settings.h"
#include "autostart.h"

Settings::Settings(QObject *parent) : QObject(parent), m_store("nothingx", "nothingx") {}

Settings::Settings(const QString &file, QObject *parent)
    : QObject(parent), m_store(file, QSettings::IniFormat) {}

void Settings::setValue(const QString &key, const QVariant &value) {
    if (m_store.value(key) == value)
        return;
    m_store.setValue(key, value);
    emit changed();
}

QString Settings::appearance() const { return m_store.value("appearance", "system").toString(); }
void Settings::setAppearance(const QString &mode) {
    if (mode == appearance())
        return;
    m_store.setValue("appearance", mode);
    emit appearanceChanged();
}

bool Settings::systemAccent() const { return m_store.value("systemAccent", false).toBool(); }
void Settings::setSystemAccent(bool on) {
    if (on == systemAccent())
        return;
    m_store.setValue("systemAccent", on);
    emit appearanceChanged();
}

// The truth lives in the OS (autostart .desktop file / Run registry key), so
// read it from there rather than mirroring it.
bool Settings::launchAtLogin() const { return Autostart::isEnabled(); }
void Settings::setLaunchAtLogin(bool on) {
    if (on == launchAtLogin())
        return;
    Autostart::setEnabled(on);
    emit launchAtLoginChanged();
}

bool Settings::lowBatteryNotification() const { return m_store.value("lowBatteryNotification", true).toBool(); }
void Settings::setLowBatteryNotification(bool on) { setValue("lowBatteryNotification", on); }
bool Settings::batteryInTray() const { return m_store.value("batteryInTray", true).toBool(); }
void Settings::setBatteryInTray(bool on) { setValue("batteryInTray", on); }
bool Settings::autoConnect() const { return m_store.value("autoConnect", true).toBool(); }
void Settings::setAutoConnect(bool on) { setValue("autoConnect", on); }
bool Settings::autoUpdate() const { return m_store.value("autoUpdate", true).toBool(); }
void Settings::setAutoUpdate(bool on) { setValue("autoUpdate", on); }
QString Settings::lastDeviceAddress() const { return m_store.value("lastDeviceAddress").toString(); }
void Settings::setLastDeviceAddress(const QString &a) { setValue("lastDeviceAddress", a); }
QString Settings::lastDeviceName() const { return m_store.value("lastDeviceName").toString(); }
void Settings::setLastDeviceName(const QString &n) { setValue("lastDeviceName", n); }

bool Settings::takeFirstRun() {
    if (m_store.value("firstRunDone", false).toBool())
        return false;
    m_store.setValue("firstRunDone", true);
    return true;
}

static QString deviceKey(const QString &address, const QString &key) {
    QString a = address;
    return "devices/" + a.remove(':') + "/" + key;
}

QVariant Settings::deviceValue(const QString &address, const QString &key, const QVariant &fallback) const {
    return m_store.value(deviceKey(address, key), fallback);
}

void Settings::setDeviceValue(const QString &address, const QString &key, const QVariant &value) {
    setValue(deviceKey(address, key), value);
}
