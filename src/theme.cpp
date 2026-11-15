// Watching Omarchy's theme follows the pattern of Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"
#include "settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QStyleHints>
#include <cmath>

#ifdef Q_OS_WIN
#include <QSettings>
#endif

Theme::Theme(Settings *settings, QObject *parent, const QString &omarchyColorsFile)
    : QObject(parent), m_settings(settings),
      m_colorsFile(omarchyColorsFile.isEmpty()
                       ? QDir::homePath() + "/.local/state/omarchy/current/theme/colors.toml"
                       : omarchyColorsFile) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(m_settings, &Settings::appearanceChanged, this, &Theme::reload);
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &Theme::reload);
#ifdef Q_OS_WIN
    // Windows has no change signal for the accent we can reach without a
    // native event filter; a slow poll is cheap and good enough.
    auto *poll = new QTimer(this);
    poll->setInterval(5000);
    connect(poll, &QTimer::timeout, this, &Theme::reload);
    poll->start();
#endif
    reload();
}

QString Theme::systemName() const {
#ifdef Q_OS_WIN
    return QStringLiteral("Windows");
#else
    return QFileInfo::exists(m_colorsFile) ? QStringLiteral("Omarchy") : QStringLiteral("system");
#endif
}

QColor Theme::readOmarchyAccent(const QString &path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        static const QRegularExpression re(R"re(^\s*accent\s*=\s*["'](#[0-9a-fA-F]{6})["'])re",
                                           QRegularExpression::MultilineOption);
        const auto match = re.match(QString::fromUtf8(file.readAll()));
        if (match.hasMatch())
            return QColor(match.captured(1));
    }
    return {};
}

bool Theme::readOmarchyDark(const QString &path, bool fallback) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        static const QRegularExpression re(R"re(^\s*mode\s*=\s*["'](dark|light)["'])re",
                                           QRegularExpression::MultilineOption);
        const auto match = re.match(QString::fromUtf8(file.readAll()));
        if (match.hasMatch())
            return match.captured(1) == "dark";
    }
    return fallback;
}

QColor Theme::systemAccent() const {
#ifdef Q_OS_WIN
    // DWM stores the accent as 0xAABBGGRR.
    QSettings dwm("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\DWM", QSettings::NativeFormat);
    bool ok = false;
    const uint abgr = dwm.value("AccentColor").toUInt(&ok);
    if (ok)
        return QColor(abgr & 0xFF, (abgr >> 8) & 0xFF, (abgr >> 16) & 0xFF);
    return {};
#else
    return readOmarchyAccent(m_colorsFile);
#endif
}

bool Theme::systemDark() const {
    const bool qtDark = qGuiApp->styleHints()->colorScheme() != Qt::ColorScheme::Light;
#ifdef Q_OS_WIN
    return qtDark;
#else
    // Omarchy says which mode its theme is; desktops without it fall back
    // to what Qt was told by the platform.
    return readOmarchyDark(m_colorsFile, qtDark);
#endif
}

// Black or white, whichever reads on the accent (WCAG relative luminance).
QColor Theme::onAccent() const {
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    const double l = .2126 * linear(m_accent.redF()) + .7152 * linear(m_accent.greenF()) +
                     .0722 * linear(m_accent.blueF());
    return l > .179 ? QColor(Qt::black) : QColor(Qt::white);
}

void Theme::rearmWatcher() {
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);
#ifndef Q_OS_WIN
    // A theme switch replaces symlinks, so watch the parents and re-arm.
    QStringList candidates;
    QString dir = QFileInfo(m_colorsFile).absolutePath();
    for (int i = 0; i < 3 && dir.size() > 1; ++i) {
        candidates << dir;
        dir = QFileInfo(dir).absolutePath();
    }
    candidates << m_colorsFile;
    for (const auto &p : candidates)
        if (QFileInfo::exists(p))
            m_watcher.addPath(p);
#endif
}

void Theme::reload() {
    rearmWatcher();
    const QString mode = m_settings->appearance();
    const bool dark = mode == "dark" ? true : mode == "light" ? false : systemDark();

    QColor accent(NothingRed);
    if (m_settings->systemAccent()) {
        const QColor sys = systemAccent();
        if (sys.isValid())
            accent = sys;
    }
    if (dark != m_dark || accent != m_accent) {
        m_dark = dark;
        m_accent = accent;
        emit changed();
    }
}
