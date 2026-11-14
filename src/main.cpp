// Nothing X: the Nothing X app's controls for Nothing earbuds, in the tray.

#include "app.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char *argv[]) {
    // The tray icon needs QApplication (QSystemTrayIcon lives in QtWidgets).
    QApplication app(argc, argv);
    app.setApplicationName("nothingx");
    app.setApplicationDisplayName("Nothing X");
    app.setOrganizationName("nothingx");
    app.setApplicationVersion(NOTHINGX_VERSION);
    app.setDesktopFileName("nothingx");
    app.setWindowIcon(QIcon(":/icons/app.svg"));
    // Closing the popup must never end the app: it lives in the tray.
    app.setQuitOnLastWindowClosed(false);
    QQuickStyle::setStyle("Basic");

    QCommandLineParser parser;
    parser.setApplicationDescription("Control Nothing earbuds from the tray.");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption hidden("hidden", "Start in the tray without opening the popup.");
    QCommandLineOption toggle("toggle", "Toggle the popup of the running instance.");
    QCommandLineOption show("show", "Show the popup of the running instance.");
    QCommandLineOption quit("quit", "Quit the running instance.");
    QCommandLineOption demo("demo", "Use simulated earbuds (no Bluetooth needed).");
    QCommandLineOption screenshots("screenshots", "Developer aid: save every page as PNG into DIR and exit.", "DIR");
    parser.addOptions({hidden, toggle, show, quit, demo, screenshots});
    parser.process(app);

    const QString command = parser.isSet(quit) ? "quit" : parser.isSet(show) ? "show" : "toggle";
    if (!parser.isSet(screenshots) && App::sendToRunning(command))
        return 0;
    if (parser.isSet(quit))
        return 0;

    App::Options options;
    options.hidden = parser.isSet(hidden);
    options.demo = parser.isSet(demo) || parser.isSet(screenshots);
    options.screenshotDir = parser.value(screenshots);
    App nothingx(options);
    if (!nothingx.start())
        return 1;
    return app.exec();
}
