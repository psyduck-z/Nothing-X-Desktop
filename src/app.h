#pragma once

#include <QLocalServer>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <memory>

class Device;
class DeviceScanner;
class OfficialPack;
class PopupPlacer;
class Settings;
class Theme;
class TrayIcon;
class Updater;

// Wires the pieces together: one tray icon, one popup window, one device,
// the official app's look and data, and the updater.
class App : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString platform READ platform CONSTANT)
    Q_PROPERTY(bool demo READ demo CONSTANT)

public:
    struct Options {
        bool hidden = false;
        bool demo = false;
        // Developer aid: render every page into this directory and quit.
        QString screenshotDir;
    };
    explicit App(const Options &options, QObject *parent = nullptr);
    ~App() override;

    bool start();
    // Handles a command from a second launch ("toggle", "show", "hide",
    // "quit", "reload-official").
    void handleCommand(const QString &command);

    QString version() const;
    QString platform() const;
    bool demo() const { return m_options.demo; }

    Q_INVOKABLE void hidePopup();
    Q_INVOKABLE void quit();
    Q_INVOKABLE void openBluetoothSettings();
    // Asks for an .xapk/.apk of the official app and imports it.
    Q_INVOKABLE void chooseOfficialApp();
    // Runs the assistant command (also what the earbuds gesture does).
    Q_INVOKABLE void runAssistant();

    static QString serverName();
    // Sends `command` to a running instance; true if one answered.
    static bool sendToRunning(const QString &command);

private:
    void takeScreenshots();
    Options m_options;
    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<Theme> m_theme;
    std::unique_ptr<OfficialPack> m_pack;
    std::unique_ptr<Device> m_device;
    std::unique_ptr<DeviceScanner> m_scanner;
    std::unique_ptr<Updater> m_updater;
    std::unique_ptr<TrayIcon> m_tray;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    std::unique_ptr<PopupPlacer> m_placer;
    QLocalServer m_server;
    QTimer m_officialWatch;
};
