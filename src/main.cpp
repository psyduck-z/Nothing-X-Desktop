// Nothing X: the Nothing X app's controls for Nothing earbuds, in the tray.

#include "app.h"
#include "officialpack.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char *argv[]) {
    // Importing the official app is a terminal command (bin/install runs it):
    // it needs no window, so it doesn't need a display either.
    for (int i = 1; i < argc; ++i)
        if (qstrcmp(argv[i], "--import-official") == 0)
            qputenv("QT_QPA_PLATFORM", "offscreen");

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
    QCommandLineOption hide("hide", "Hide the popup of the running instance.");
    QCommandLineOption quit("quit", "Quit the running instance.");
    QCommandLineOption demo("demo", "Use simulated earbuds (no Bluetooth needed).");
    QCommandLineOption screenshots("screenshots", "Developer aid: save every page as PNG into DIR and exit.", "DIR");
    QCommandLineOption importOfficial("import-official",
                                      "Import the look and device data of the official Nothing X app from FILE (.xapk or .apk).",
                                      "FILE");
    parser.addOptions({hidden, toggle, show, hide, quit, demo, screenshots, importOfficial});
    parser.process(app);

    if (parser.isSet(importOfficial)) {
        QString version, error;
        if (!OfficialPack::unpack(parser.value(importOfficial), OfficialPack::defaultCacheRoot(), &version, &error)) {
            fprintf(stderr, "nothingx: %s\n", qPrintable(error));
            return 1;
        }
        printf("Imported the official Nothing X %s.\n", qPrintable(version));
        // A running instance picks the new version up straight away.
        App::sendToRunning("reload-official");
        return 0;
    }

    const QString command = parser.isSet(quit) ? "quit" : parser.isSet(show) ? "show" : parser.isSet(hide) ? "hide" : "toggle";
    if (!parser.isSet(screenshots) && App::sendToRunning(command))
        return 0;
    if (parser.isSet(quit) || parser.isSet(hide))
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
