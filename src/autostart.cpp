#include "autostart.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

namespace Autostart {

#ifdef Q_OS_WIN
static const char *RunKey = "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool isEnabled() {
    QSettings run(RunKey, QSettings::NativeFormat);
    return run.contains("Nothing X");
}

void setEnabled(bool on) {
    QSettings run(RunKey, QSettings::NativeFormat);
    if (on)
        run.setValue("Nothing X", "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\" --hidden");
    else
        run.remove("Nothing X");
}

QString linuxEntryPath() { return {}; }
#else
QString linuxEntryPath() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart/nothingx.desktop";
}

bool isEnabled() { return QFileInfo::exists(linuxEntryPath()); }

void setEnabled(bool on) {
    const QString path = linuxEntryPath();
    if (!on) {
        QFile::remove(path);
        return;
    }
    QDir().mkpath(QFileInfo(path).absolutePath());
    // Prefer the installed binary so the entry survives rebuilds in place.
    const QString exe = QFileInfo::exists("/usr/bin/nothingx") ? QStringLiteral("/usr/bin/nothingx")
                                                               : QCoreApplication::applicationFilePath();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write("[Desktop Entry]\n"
               "Type=Application\n"
               "Name=Nothing X\n"
               "Comment=Nothing earbuds in the tray\n"
               "Icon=nothingx\n"
               "Exec=" + exe.toUtf8() + " --hidden\n"
               "X-GNOME-Autostart-enabled=true\n");
    file.commit();
}
#endif

} // namespace Autostart
