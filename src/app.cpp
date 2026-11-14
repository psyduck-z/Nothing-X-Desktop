#include "app.h"
#include "autostart.h"
#include "bttransport.h"
#include "demotransport.h"
#include "device.h"
#include "devicescanner.h"
#include "iconprovider.h"
#include "popupplacer.h"
#include "settings.h"
#include "theme.h"
#include "trayicon.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QLocalSocket>
#include <QProcess>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <functional>
#include <memory>

App::App(const Options &options, QObject *parent) : QObject(parent), m_options(options) {}

App::~App() {
    // The engine holds bindings to the objects below; tear it down first.
    m_placer.reset();
    m_engine.reset();
    m_tray.reset();
}

QString App::version() const { return QStringLiteral(NOTHINGX_VERSION); }

QString App::platform() const {
#ifdef Q_OS_WIN
    return QStringLiteral("windows");
#else
    return QStringLiteral("linux");
#endif
}

QString App::serverName() {
    return QStringLiteral("nothingx-") + qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME", "user"));
}

bool App::sendToRunning(const QString &command) {
    QLocalSocket socket;
    socket.connectToServer(serverName());
    if (!socket.waitForConnected(300))
        return false;
    socket.write(command.toUtf8() + '\n');
    socket.waitForBytesWritten(300);
    socket.disconnectFromServer();
    return true;
}

bool App::start() {
    // The demo (and the screenshots) keep their own settings, so the
    // simulated earbuds never become the "last device" of the real app.
    m_settings = m_options.demo ? std::make_unique<Settings>(QDir::tempPath() + "/nothingx-demo.conf", nullptr)
                                : std::make_unique<Settings>();
    const bool realRun = !m_options.demo && m_options.screenshotDir.isEmpty();
    // Nothing X lives in the tray, so it starts with the computer unless the
    // user turns that off in App settings. On later runs, rewrite an existing
    // entry so it follows the binary if it moved (build tree to /usr/bin).
    if (realRun && m_settings->takeFirstRun())
        m_settings->setLaunchAtLogin(true);
    else if (realRun && m_settings->launchAtLogin())
        Autostart::setEnabled(true);
    m_theme = std::make_unique<Theme>(m_settings.get());

    Transport *transport = m_options.demo ? static_cast<Transport *>(new DemoTransport)
                                          : static_cast<Transport *>(new BluetoothTransport);
    m_device = std::make_unique<Device>(transport, m_settings.get());
    m_scanner = std::make_unique<DeviceScanner>();
    m_tray = std::make_unique<TrayIcon>(m_device.get(), m_settings.get(), m_scanner.get());

    m_engine = std::make_unique<QQmlApplicationEngine>();
    m_engine->addImageProvider("icon", new IconProvider);
    auto *ctx = m_engine->rootContext();
    ctx->setContextProperty("app", this);
    ctx->setContextProperty("settings", m_settings.get());
    ctx->setContextProperty("theme", m_theme.get());
    ctx->setContextProperty("device", m_device.get());
    ctx->setContextProperty("scanner", m_scanner.get());
    m_engine->load(QUrl("qrc:/qml/Main.qml"));
    if (m_engine->rootObjects().isEmpty())
        return false;
    auto *window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().constFirst());
    if (!window)
        return false;

    m_placer = std::make_unique<PopupPlacer>(window, m_tray->tray());
    connect(m_tray.get(), &TrayIcon::toggleRequested, m_placer.get(), &PopupPlacer::toggle);
    connect(m_tray.get(), &TrayIcon::showRequested, m_placer.get(), &PopupPlacer::show);
    connect(m_tray.get(), &TrayIcon::quitRequested, this, &App::quit);

    // A second launch (app launcher, keybinding) toggles this instance's
    // popup instead of starting another tray icon.
    QLocalServer::removeServer(serverName());
    m_server.listen(serverName());
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::readyRead, socket, [this, socket] {
                while (socket->canReadLine())
                    handleCommand(QString::fromUtf8(socket->readLine()).trimmed());
            });
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    // Reconnect to the last earbuds whenever they connect to the computer.
    connect(m_scanner.get(), &DeviceScanner::deviceConnected, this, [this](const QString &address, const QString &name) {
        if (m_settings->autoConnect() && m_device->state() == Device::Disconnected &&
            address == m_settings->lastDeviceAddress())
            m_device->connectToDevice(address, name);
    });

    if (m_options.demo) {
        m_device->connectToDevice("00:11:22:33:44:55", "Nothing Ear (3)");
    } else {
        m_scanner->refresh();
        // Connect whenever the last earbuds are connected to the computer
        // (out of the case, back after an LDAC restart), but don't report an
        // error while they are simply away.
        auto *watch = new QTimer(this);
        watch->setInterval(5000);
        auto tryConnect = [this] {
            const QString last = m_settings->lastDeviceAddress();
            if (m_settings->autoConnect() && !last.isEmpty() && m_device->state() == Device::Disconnected &&
                m_scanner->isConnected(last))
                m_device->connectToLast();
        };
        connect(watch, &QTimer::timeout, this, tryConnect);
        watch->start();
        tryConnect();
    }

    if (!m_options.screenshotDir.isEmpty()) {
        window->show();
        QTimer::singleShot(600, this, &App::takeScreenshots);
    } else if (!m_options.hidden) {
        QTimer::singleShot(150, m_placer.get(), &PopupPlacer::show);
    }
    return true;
}

void App::handleCommand(const QString &command) {
    if (command == "show")
        m_placer->show();
    else if (command == "hide")
        m_placer->hide();
    else if (command == "quit")
        quit();
    else
        m_placer->toggle();
}

void App::hidePopup() { m_placer->hide(); }

void App::quit() {
    m_device->disconnectFromDevice();
    QTimer::singleShot(0, qApp, &QApplication::quit);
}

void App::openBluetoothSettings() {
    m_placer->hide();
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl("ms-settings:bluetooth"));
#else
    // Omarchy's own Bluetooth launcher, then the common Linux managers.
    for (const auto &[program, args] : QList<QPair<QString, QStringList>>{
             {"omarchy-launch-bluetooth", {}},
             {"blueman-manager", {}},
             {"xdg-terminal-exec", {"bluetui"}},
         }) {
        if (!QStandardPaths::findExecutable(program).isEmpty()) {
            QProcess::startDetached(program, args);
            return;
        }
    }
#endif
}

void App::takeScreenshots() {
    auto *window = qobject_cast<QQuickWindow *>(m_engine->rootObjects().constFirst());
    // Every look of the tray icon, at the sizes Windows and Linux bars use.
    for (bool light : {false, true})
        for (int size : {16, 22, 32, 64}) {
            const QString prefix = m_options.screenshotDir + QStringLiteral("/tray-%1-%2-").arg(light ? "light" : "dark").arg(size);
            TrayIcon::renderImage(TrayIcon::Disconnected, -1, light, size).save(prefix + "off.png");
            TrayIcon::renderImage(TrayIcon::Earbuds, -1, light, size).save(prefix + "earbuds.png");
            for (int level : {100, 85, 42, 15, 3})
                TrayIcon::renderImage(TrayIcon::Battery, level, light, size).save(prefix + QString::number(level) + ".png");
        }
    const QStringList pages{"home", "eq", "sound", "gestures", "mic", "fit", "find", "dual", "about", "devices", "settings"};
    auto index = std::make_shared<int>(0);
    auto step = std::make_shared<std::function<void()>>();
    *step = [this, window, pages, index, step] {
        const QString name = pages.value(*index);
        window->grabWindow().save(m_options.screenshotDir + "/" + QString("%1-%2.png").arg(*index, 2, 10, QChar('0')).arg(name));
        QMetaObject::invokeMethod(window, "closeAll");
        if (++*index >= pages.size()) {
            QApplication::quit();
            return;
        }
        QMetaObject::invokeMethod(window, "open", Q_ARG(QVariant, pages[*index]));
        QTimer::singleShot(450, this, *step);
    };
    (*step)();
}
