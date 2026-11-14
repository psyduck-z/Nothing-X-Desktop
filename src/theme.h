#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>

class Settings;

// The palette the QML reads. It is Nothing's own look (black, white, grey,
// one red) on every platform, so the Omarchy and Windows builds look the
// same; the accent can optionally follow the system: Omarchy's theme accent
// on Linux, the Windows accent color on Windows.
class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor background READ background NOTIFY changed)
    Q_PROPERTY(QColor surface READ surface NOTIFY changed)
    Q_PROPERTY(QColor surfaceRaised READ surfaceRaised NOTIFY changed)
    Q_PROPERTY(QColor outline READ outline NOTIFY changed)
    Q_PROPERTY(QColor text READ text NOTIFY changed)
    Q_PROPERTY(QColor textMuted READ textMuted NOTIFY changed)
    Q_PROPERTY(QColor onAccent READ onAccent NOTIFY changed)
    Q_PROPERTY(QColor inverse READ inverse NOTIFY changed)
    Q_PROPERTY(QColor onInverse READ onInverse NOTIFY changed)
    Q_PROPERTY(QString systemName READ systemName CONSTANT)

public:
    explicit Theme(Settings *settings, QObject *parent = nullptr,
                   const QString &omarchyColorsFile = {});

    bool dark() const { return m_dark; }
    QColor accent() const { return m_accent; }
    QColor background() const { return m_dark ? QColor("#000000") : QColor("#F2F2F2"); }
    QColor surface() const { return m_dark ? QColor("#141414") : QColor("#FFFFFF"); }
    QColor surfaceRaised() const { return m_dark ? QColor("#1F1F1F") : QColor("#E9E9E9"); }
    QColor outline() const { return m_dark ? QColor("#2C2C2C") : QColor("#D6D6D6"); }
    QColor text() const { return m_dark ? QColor("#FFFFFF") : QColor("#000000"); }
    QColor textMuted() const { return m_dark ? QColor("#8C8C8C") : QColor("#6B6B6B"); }
    QColor inverse() const { return text(); }
    QColor onInverse() const { return background(); }
    QColor onAccent() const;
    QString systemName() const;

    static QColor readOmarchyAccent(const QString &path);
    static bool readOmarchyDark(const QString &path, bool fallback);

    static constexpr const char *NothingRed = "#D71921";

signals:
    void changed();

private:
    void reload();
    void rearmWatcher();
    QColor systemAccent() const;
    bool systemDark() const;

    Settings *m_settings;
    QString m_colorsFile;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
    bool m_dark = true;
    QColor m_accent{NothingRed};
};
