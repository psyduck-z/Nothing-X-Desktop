#include "updater.h"
#include "officialpack.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>

Updater::Updater(const QString &currentVersion, QObject *parent) : QObject(parent), m_current(currentVersion) {
    m_timer.setInterval(12 * 60 * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, &Updater::check);
}

QString Updater::releasesUrl() {
    return QStringLiteral("https://api.github.com/repos/%1/releases/latest").arg(QStringLiteral(NOTHINGX_REPO));
}

bool Updater::updateAvailable() const {
    return !m_latest.isEmpty() && OfficialPack::compareVersions(m_latest, m_current) > 0 && !m_asset.isEmpty();
}

bool Updater::canInstall() const {
#ifdef Q_OS_WIN
    return true;
#else
    // Installed as a package; a source checkout updates with git and bin/install.
    return QCoreApplication::applicationFilePath() == QLatin1String("/usr/bin/nothingx");
#endif
}

void Updater::startAutomaticChecks() {
    if (m_timer.isActive())
        return;
    QTimer::singleShot(20 * 1000, this, &Updater::check);
    m_timer.start();
}

void Updater::setStatus(const QString &status, bool busy) {
    m_status = status;
    m_busy = busy;
    emit changed();
}

QJsonObject Updater::pickAsset(const QJsonArray &assets, bool windows) {
    for (const QJsonValue &v : assets) {
        const QString name = v.toObject().value("name").toString();
        if (windows ? (name.endsWith(".exe", Qt::CaseInsensitive) && name.contains("Setup", Qt::CaseInsensitive))
                    : name.endsWith(".pkg.tar.zst"))
            return v.toObject();
    }
    return {};
}

void Updater::check() {
    if (m_busy)
        return;
    setStatus(tr("Checking for updates…"), true);
    QNetworkRequest request{QUrl(releasesUrl())};
    // A generic client name only; nothing about the user is sent.
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NothingX-Desktop/%1").arg(m_current));
    request.setRawHeader("Accept", "application/vnd.github+json");
    QNetworkReply *reply = m_net.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 404) {
            setStatus(tr("No releases published yet"), false);
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            setStatus(tr("Couldn't check for updates"), false);
            return;
        }
        const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
        QString tag = release.value("tag_name").toString();
        if (tag.startsWith('v'))
            tag.remove(0, 1);
        m_latest = tag;
        m_notes = release.value("body").toString();
#ifdef Q_OS_WIN
        m_asset = pickAsset(release.value("assets").toArray(), true);
#else
        m_asset = pickAsset(release.value("assets").toArray(), false);
#endif
        if (updateAvailable()) {
            setStatus(tr("Version %1 is available").arg(m_latest), false);
            emit updateFound(m_latest);
        } else {
            setStatus(tr("Nothing X is up to date"), false);
        }
    });
}

void Updater::install() {
    if (m_busy || !updateAvailable() || !canInstall())
        return;
    setStatus(tr("Downloading %1…").arg(m_latest), true);
    QNetworkRequest request(QUrl(m_asset.value("browser_download_url").toString()));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NothingX-Desktop/%1").arg(m_current));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = m_net.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            setStatus(tr("The download failed"), false);
            return;
        }
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/updates";
        QDir().mkpath(dir);
        const QString file = dir + '/' + QFileInfo(m_asset.value("name").toString()).fileName();
        QSaveFile out(file);
        if (!out.open(QIODevice::WriteOnly) || out.write(reply->readAll()) < 0 || !out.commit()) {
            setStatus(tr("Couldn't save the update"), false);
            return;
        }
        runInstaller(file);
    });
}

void Updater::runInstaller(const QString &file) {
#ifdef Q_OS_WIN
    // The installer closes this app, replaces it and starts it again.
    setStatus(tr("Installing %1…").arg(m_latest), true);
    if (!QProcess::startDetached(file, {"/SILENT", "/SUPPRESSMSGBOXES", "/NORESTART", "/CLOSEAPPLICATIONS"})) {
        setStatus(tr("Couldn't start the installer"), false);
        return;
    }
    emit restartRequested();
#else
    setStatus(tr("Installing %1… (enter your password)").arg(m_latest), true);
    auto *pacman = new QProcess(this);
    connect(pacman, &QProcess::finished, this, [this, pacman](int exitCode, QProcess::ExitStatus status) {
        pacman->deleteLater();
        if (status != QProcess::NormalExit || exitCode != 0) {
            setStatus(tr("The update wasn't installed"), false);
            return;
        }
        // Start the new version once this one has let go of the tray.
        QProcess::startDetached("sh", {"-c", "sleep 1; exec /usr/bin/nothingx --hidden"});
        emit restartRequested();
    });
    pacman->start("pkexec", {"pacman", "-U", "--noconfirm", file});
#endif
}
