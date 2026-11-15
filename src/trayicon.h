#pragma once

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

class Device;
class DeviceScanner;
class Settings;

// The tray icon: left click toggles the popup, right click opens a short
// menu with the quick actions (noise control, connect, quit). While the
// earbuds are connected the icon is their battery level as a dot-matrix
// number; while they are not (or Bluetooth is off) it is the earbuds glyph
// struck through.
class TrayIcon : public QObject {
    Q_OBJECT
public:
    enum Look { Disconnected, Earbuds, Battery };

    TrayIcon(Device *device, Settings *settings, DeviceScanner *scanner, QObject *parent = nullptr);

    QSystemTrayIcon *tray() { return &m_tray; }
    void notify(const QString &title, const QString &message);

    // Pure painting, exposed for tests and the README screenshot.
    static QIcon renderIcon(Look look, int battery, bool lightTaskbar);
    static QImage renderImage(Look look, int battery, bool lightTaskbar, int size);
    // The number the icon shows: the level rounded to the nearest 5%.
    static int roundedLevel(int battery);

signals:
    void toggleRequested();
    void showRequested();
    void quitRequested();

private:
    void refresh();
    void rebuildMenu();
    static bool lightTaskbar();

    Device *m_device;
    Settings *m_settings;
    DeviceScanner *m_scanner;
    QSystemTrayIcon m_tray;
    QMenu m_menu;
    int m_lastLowNotified = 101;
};
