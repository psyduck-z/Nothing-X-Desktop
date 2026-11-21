#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>

class QNetworkReply;

// Keeps the desktop app current from this project's GitHub releases. Every
// release carries the Windows installer and the Arch package built from the
// same tag, so both platforms always run the same version. On Windows the
// installer runs quietly and restarts the app; on Omarchy the package is
// installed with pacman (through the system's password prompt).
//
// Every push to main publishes a release (.github/workflows/build.yml), and
// those builds are marked NOTHINGX_RELEASE_BUILD. Only they look for updates
// on their own: a build from a source checkout updates with git pull and
// bin/install, and would otherwise be told about every push.
class Updater : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString releaseNotes READ releaseNotes NOTIFY changed)
    Q_PROPERTY(bool canInstall READ canInstall CONSTANT)
    Q_PROPERTY(bool releaseBuild READ releaseBuild CONSTANT)

public:
    explicit Updater(const QString &currentVersion, QObject *parent = nullptr);

    QString currentVersion() const { return m_current; }
    QString latestVersion() const { return m_latest; }
    bool updateAvailable() const;
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    QString releaseNotes() const { return m_notes; }
    // False when running from a source checkout (update with git and bin/install).
    bool canInstall() const;

    // Built by CI for a release, rather than from a source checkout.
    static bool releaseBuild();

    // Checks shortly after start, then every hour.
    void startAutomaticChecks();
    void stopAutomaticChecks() { m_timer.stop(); }
    Q_INVOKABLE void check();
    Q_INVOKABLE void install();

    // The release file for this platform, from the GitHub API's asset list.
    static QJsonObject pickAsset(const QJsonArray &assets, bool windows);
    static QString releasesUrl();

signals:
    void changed();
    void updateFound(const QString &version);
    // The new version is installed and the app should restart.
    void restartRequested();

private:
    void setStatus(const QString &status, bool busy);
    void runInstaller(const QString &file);

    QString m_current, m_latest, m_notes, m_status;
    QJsonObject m_asset;
    bool m_busy = false;
    QNetworkAccessManager m_net;
    QTimer m_timer;
};
