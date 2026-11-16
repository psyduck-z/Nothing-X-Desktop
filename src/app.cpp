#include "app.h"
#include "autostart.h"
#include "bttransport.h"
#include "demotransport.h"
#include "device.h"
#include "devicescanner.h"
#include "officialimageprovider.h"
#include "officialpack.h"
#include "popupplacer.h"
#include "settings.h"
#include "theme.h"
#include "trayicon.h"
#include "updater.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
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
    // user turns that off. Later runs rewrite an existing entry so it follows
    // the binary if it moved (from a build tree to /usr/bin, say).
    if (realRun && m_settings->takeFirstRun())
        m_settings->setLaunchAtLogin(true);
    else if (realRun && m_settings->launchAtLogin())
        Autostart::setEnabled(true);

    m_theme = std::make_unique<Theme>(m_settings.get());
    m_pack = std::make_unique<OfficialPack>();
    Transport *transport = m_options.demo ? static_cast<Transport *>(new DemoTransport)
                                          : static_cast<Transport *>(new BluetoothTransport);
    m_device = std::make_unique<Device>(transport, m_settings.get(), m_pack.get());
    m_scanner = std::make_unique<DeviceScanner>();
    m_updater = std::make_unique<Updater>(version());
    m_tray = std::make_unique<TrayIcon>(m_device.get(), m_settings.get(), m_scanner.get());

    m_engine = std::make_unique<QQmlApplicationEngine>();
    m_engine->addImageProvider("official", new OfficialImageProvider(m_pack.get()));
    auto *ctx = m_engine->rootContext();
    ctx->setContextProperty("app", this);
    ctx->setContextProperty("settings", m_settings.get());
    ctx->setContextProperty("theme", m_theme.get());
    ctx->setContextProperty("official", m_pack.get());
    ctx->setContextProperty("device", m_device.get());
    ctx->setContextProperty("scanner", m_scanner.get());
    ctx->setContextProperty("updater", m_updater.get());
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

    // A second launch (app launcher, keybinding) talks to this instance
    // instead of starting another tray icon. The demo, the screenshots and
    // the tests leave the real app's socket alone.
    if (realRun) {
        QLocalServer::removeServer(serverName());
        m_server.listen(serverName());
    }
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::readyRead, socket, [this, socket] {
                while (socket->canReadLine())
                    handleCommand(QString::fromUtf8(socket->readLine()).trimmed());
            });
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
        }
    });

    // Updates of this app: both platforms install the same GitHub release.
    connect(m_updater.get(), &Updater::updateFound, this, [this](const QString &v) {
        m_tray->notify(tr("Nothing X %1 is available").arg(v), tr("Open Nothing X › App settings to update."));
    });
    connect(m_updater.get(), &Updater::restartRequested, this, &App::quit);
    auto followAutoUpdate = [this, realRun] {
        if (realRun && m_settings->autoUpdate())
            m_updater->startAutomaticChecks();
        else
            m_updater->stopAutomaticChecks();
    };
    connect(m_settings.get(), &Settings::changed, this, followAutoUpdate);
    followAutoUpdate();

    // New official app versions: a newer .xapk/.apk dropped into Downloads
    // (or the Desktop, Documents, home) is picked up on its own.
    connect(m_pack.get(), &OfficialPack::importFinished, this, [this](bool ok, const QString &message) {
        m_tray->notify(ok ? tr("Official app imported") : tr("Couldn't import the official app"), message);
    });
    if (realRun) {
        m_pack->importFromKnownPlaces();
        m_officialWatch.setInterval(60 * 60 * 1000);
        connect(&m_officialWatch, &QTimer::timeout, m_pack.get(), &OfficialPack::importFromKnownPlaces);
        m_officialWatch.start();
    }

    // A gesture set to "Desktop assistant" sends an event over the control
    // link (the action the official app uses for News Reporter).
    connect(m_device.get(), &Device::gestureEvent, this, [this](int value) {
        if (value == 31)
            runAssistant();
    });

    // Reconnect to the last earbuds whenever they connect to the computer.
    // The first time, with nothing chosen yet, take the only paired Nothing
    // earbuds as soon as they connect.
    connect(m_scanner.get(), &DeviceScanner::deviceConnected, this, [this](const QString &address, const QString &name) {
        if (!m_settings->autoConnect() || m_device->state() != Device::Disconnected)
            return;
        const QString last = m_settings->lastDeviceAddress();
        if (address == last || (last.isEmpty() && m_scanner->devices().size() == 1))
            m_device->connectToDevice(address, name);
    });

    if (m_options.demo) {
        m_device->connectToDevice("00:11:22:33:44:55", "Nothing Ear (3)");
    } else {
        m_scanner->refresh();
        // Connect whenever the last earbuds are connected to the computer
        // (out of the case, back after a restart), without reporting an
        // error while they are simply away.
        auto *watch = new QTimer(this);
        watch->setInterval(5000);
        auto tryConnect = [this] {
            if (!m_settings->autoConnect() || m_device->state() != Device::Disconnected)
                return;
            const QString last = m_settings->lastDeviceAddress();
            if (!last.isEmpty()) {
                if (m_scanner->isConnected(last))
                    m_device->connectToLast();
                return;
            }
            const QVariantList found = m_scanner->devices();
            if (found.size() == 1 && found.first().toMap().value("connected").toBool())
                m_device->connectToDevice(found.first().toMap().value("address").toString(),
                                          found.first().toMap().value("name").toString());
        };
        connect(watch, &QTimer::timeout, this, tryConnect);
        watch->start();
        tryConnect();
    }

    if (!m_options.screenshotDir.isEmpty()) {
        window->show();
        QTimer::singleShot(900, this, &App::takeScreenshots);
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
    else if (command == "reload-official")
        m_pack->reload();
    else
        m_placer->toggle();
}

void App::hidePopup() { m_placer->hide(); }

void App::quit() {
    m_device->disconnectFromDevice();
    QTimer::singleShot(0, qApp, &QApplication::quit);
}

QString App::voiceCommand(const QString &exe) {
    return QStringLiteral("\"%1\" --voice").arg(QDir::toNativeSeparators(exe));
}

QString App::cmdArguments(const QString &command) {
    return QStringLiteral("/d /s /c \"%1\"").arg(command);
}

QString App::assistantFallback() const {
#ifdef Q_OS_WIN
    // Wumpy's installer is per user and puts it here unless told otherwise;
    // it adds nothing to PATH, so a bare "wumpy" would not be found.
    const QString exe = QDir(qEnvironmentVariable("LOCALAPPDATA")).filePath(QStringLiteral("Programs/Wumpy/Wumpy.exe"));
#else
    const QString exe = QDir::home().filePath(QStringLiteral(".local/bin/wumpy"));
#endif
    return QFileInfo::exists(exe) ? voiceCommand(exe) : QString();
}

void App::runAssistant() {
    QString command = m_settings->assistantCommand();
    if (command.isEmpty())
        command = assistantFallback();
    if (command.isEmpty()) {
        m_tray->notify(tr("Desktop assistant"), tr("Choose what the gesture runs in Nothing X › App settings › Assistant."));
        return;
    }
#ifdef Q_OS_WIN
    // Through cmd, so the command can be anything that runs at a prompt.
    // Given to cmd as an ordinary argument, Qt would escape the command's
    // own quotes as \", which cmd does not read as quotes, so a quoted path
    // with a space in it - most of them, on Windows - would not run. /s /c
    // with the command in one more pair of quotes is the form cmd strips
    // back to exactly what was typed. No window, so a gesture does not
    // flash a console.
    constexpr unsigned long CreateNoWindow = 0x08000000; // CREATE_NO_WINDOW
    QProcess process;
    process.setProgram(QStringLiteral("cmd.exe"));
    process.setNativeArguments(cmdArguments(command));
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CreateNoWindow;
    });
    const bool started = process.startDetached();
#else
    const bool started = QProcess::startDetached("sh", {"-c", command});
#endif
    if (!started)
        m_tray->notify(tr("Desktop assistant"), tr("Couldn't run: %1").arg(command));
}

void App::chooseOfficialApp() {
    m_placer->hide();
    const QString file = QFileDialog::getOpenFileName(
        nullptr, tr("Choose the Nothing X app"),
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
        tr("Android apps (*.xapk *.apk)"));
    if (!file.isEmpty())
        m_pack->importFile(file);
    m_placer->show();
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
    const QStringList pages = window->property("pageNames").toStringList();
    // Developer aid: a taller window shows whole pages.
    if (const int h = qEnvironmentVariableIntValue("NOTHINGX_SHOT_HEIGHT"); h > 0)
        window->setHeight(h);
    auto index = std::make_shared<int>(0);
    auto step = std::make_shared<std::function<void()>>();
    *step = [this, window, pages, index, step] {
        const QString name = pages.value(*index, "home");
        window->grabWindow().save(m_options.screenshotDir + "/" + QString("%1-%2.png").arg(*index, 2, 10, QChar('0')).arg(name));
        QMetaObject::invokeMethod(window, "closeAll");
        if (++*index >= pages.size()) {
            QApplication::quit();
            return;
        }
        QMetaObject::invokeMethod(window, "open", Q_ARG(QVariant, pages[*index]), Q_ARG(QVariant, QVariant()));
        QTimer::singleShot(600, this, *step);
    };
    // Give the window a moment to lay out at its screenshot size.
    QTimer::singleShot(600, this, *step);
}
