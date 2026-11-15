#pragma once

#include <QObject>
#include <QSettings>

// App preferences, stored with QSettings: ~/.config/nothingx/nothingx.conf
// on Linux, HKCU\Software\nothingx on Windows.
class Settings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString appearance READ appearance WRITE setAppearance NOTIFY appearanceChanged)
    Q_PROPERTY(bool systemAccent READ systemAccent WRITE setSystemAccent NOTIFY appearanceChanged)
    Q_PROPERTY(bool launchAtLogin READ launchAtLogin WRITE setLaunchAtLogin NOTIFY launchAtLoginChanged)
    Q_PROPERTY(bool lowBatteryNotification READ lowBatteryNotification WRITE setLowBatteryNotification NOTIFY changed)
    Q_PROPERTY(bool batteryInTray READ batteryInTray WRITE setBatteryInTray NOTIFY changed)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY changed)
    Q_PROPERTY(bool autoUpdate READ autoUpdate WRITE setAutoUpdate NOTIFY changed)
    // Run when an earbuds gesture set to "Desktop assistant" fires.
    Q_PROPERTY(QString assistantCommand READ assistantCommand WRITE setAssistantCommand NOTIFY changed)
    Q_PROPERTY(QString lastDeviceAddress READ lastDeviceAddress WRITE setLastDeviceAddress NOTIFY changed)
    Q_PROPERTY(QString lastDeviceName READ lastDeviceName WRITE setLastDeviceName NOTIFY changed)

public:
    explicit Settings(QObject *parent = nullptr);
    // Tests point this at a temporary file.
    Settings(const QString &file, QObject *parent);

    QString appearance() const;
    void setAppearance(const QString &mode);
    bool systemAccent() const;
    void setSystemAccent(bool on);
    bool launchAtLogin() const;
    void setLaunchAtLogin(bool on);
    bool lowBatteryNotification() const;
    void setLowBatteryNotification(bool on);
    bool batteryInTray() const;
    void setBatteryInTray(bool on);
    bool autoConnect() const;
    void setAutoConnect(bool on);
    bool autoUpdate() const;
    void setAutoUpdate(bool on);
    QString assistantCommand() const;
    void setAssistantCommand(const QString &command);
    QString lastDeviceAddress() const;
    void setLastDeviceAddress(const QString &a);
    QString lastDeviceName() const;
    void setLastDeviceName(const QString &n);

    // True exactly once: on the very first start of the app.
    bool takeFirstRun();

    // Per-device memory of values the earbuds cannot report back (for
    // example the last custom EQ), keyed by Bluetooth address.
    Q_INVOKABLE QVariant deviceValue(const QString &address, const QString &key, const QVariant &fallback = {}) const;
    Q_INVOKABLE void setDeviceValue(const QString &address, const QString &key, const QVariant &value);

signals:
    void appearanceChanged();
    void launchAtLoginChanged();
    void changed();

private:
    void setValue(const QString &key, const QVariant &value);
    QSettings m_store;
};
