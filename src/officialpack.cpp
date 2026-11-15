#include "officialpack.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QPointer>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>

namespace {

// What the desktop app uses from the official app: device configuration,
// earbuds artwork, animations and the Nothing fonts. Watch, fitness and AI
// wallpaper assets are left out; they are most of the app's size.
const QStringList Wanted{
    "assets/flutter_assets/assets/config",
    "assets/flutter_assets/assets/images",
    "assets/flutter_assets/assets/lottie",
    "assets/flutter_assets/packages/nt_widget/assets",
    "assets/flutter_assets/fonts",
    "assets/flutter_assets/FontManifest.json",
    "assets/lottie",
    "assets/fonts",
    // Offline product photos of the Ear (3) (the online ones are bigger).
    "res/drawable/*ear_three*",
};
const QStringList Skipped{"*_cn.json", "*watch*", "*exercise*", "*fitness*", "*sport*", "*health*",
                          "*ai_wall_paper*", "*ai_coach*", "*app_migration*"};

// bsdtar reads zip files (and so .apk and .xapk files) on both platforms:
// Arch's pacman depends on it, and Windows 10 and 11 ship it as tar.exe.
QString tarProgram() {
#ifdef Q_OS_WIN
    // System32's tar is bsdtar; a GNU tar from Git on the PATH can't read zips.
    return QDir(qEnvironmentVariable("SystemRoot", "C:/Windows")).filePath("System32/tar.exe");
#else
    const QString bsdtar = QStandardPaths::findExecutable("bsdtar");
    return bsdtar.isEmpty() ? QStringLiteral("tar") : bsdtar;
#endif
}

// Extracts the named members (or directories) of a zip file into dir.
// Paths that would land outside dir are refused by bsdtar itself.
bool extract(const QString &zip, const QString &dir, const QStringList &members, const QStringList &excludes = {}) {
    QStringList args{"-x", "-f", QDir::toNativeSeparators(zip), "-C", QDir::toNativeSeparators(dir)};
    for (const QString &e : excludes)
        args << "--exclude" << e;
    args << members;
    QProcess tar;
    tar.start(tarProgram(), args);
    return tar.waitForFinished(5 * 60 * 1000) && tar.exitStatus() == QProcess::NormalExit && tar.exitCode() == 0;
}

bool safeRelative(const QString &path) {
    return !path.isEmpty() && !path.startsWith('/') && !path.contains('\\') && !path.split('/').contains("..");
}

QJsonDocument readJson(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll());
}

} // namespace

QString OfficialPack::defaultCacheRoot() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/official";
}

OfficialPack::OfficialPack(const QString &cacheRoot, QObject *parent)
    : QObject(parent), m_cacheRoot(cacheRoot.isEmpty() ? defaultCacheRoot() : cacheRoot) {
    load();
}

QString OfficialPack::versionFromName(const QString &fileName) {
    const auto m = QRegularExpression(R"((\d+\.\d+(?:\.\d+)*))").match(QFileInfo(fileName).fileName());
    return m.hasMatch() ? m.captured(1) : QString();
}

int OfficialPack::compareVersions(const QString &a, const QString &b) {
    const QStringList pa = a.split('.'), pb = b.split('.');
    for (int i = 0; i < qMax(pa.size(), pb.size()); ++i) {
        const int x = pa.value(i).toInt(), y = pb.value(i).toInt();
        if (x != y)
            return x < y ? -1 : 1;
    }
    return 0;
}

void OfficialPack::load() {
    // The newest complete version in the cache (a finished import has pack.json).
    QString best, bestVersion;
    const QDir root(m_cacheRoot);
    for (const QString &name : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QJsonObject meta = readJson(root.filePath(name + "/pack.json")).object();
        const QString v = meta.value("version").toString();
        if (v.isEmpty())
            continue;
        if (best.isEmpty() || compareVersions(v, bestVersion) > 0) {
            best = root.filePath(name);
            bestVersion = v;
        }
    }
    m_dir = best;
    m_version = bestVersion;
    m_whiteList = available() ? readJson(m_dir + "/assets/config/ear_white_list.json").array() : QJsonArray();
    m_skus = available() ? readJson(m_dir + "/assets/config/devices_info_list.json").object().value("deviceSkuResps").toArray()
                         : QJsonArray();
    registerFonts();
    emit changed();
}

void OfficialPack::registerFonts() {
    // A re-import brings the same families again; start from a clean slate.
    QFontDatabase::removeAllApplicationFonts();
    auto add = [this](const QString &relative, const QString &fallback) {
        const QString path = assetPath(relative);
        if (!path.isEmpty()) {
            const int id = QFontDatabase::addApplicationFont(path);
            const QStringList families = QFontDatabase::applicationFontFamilies(id);
            if (!families.isEmpty())
                return families.first();
        }
        return fallback;
    };
    // The same families the official app's nt_widget package declares.
    m_dotFont = add("packages/nt_widget/assets/fonts/Ndot-55-UpAndLow.ttf", "monospace");
    m_headlineFont = add("packages/nt_widget/assets/fonts/NType82-Regular.otf", "sans-serif");
    const QString inter = add("packages/nt_widget/assets/fonts/Inter-VariableFont_opsz_wght.ttf", "sans-serif");
    m_bodyFont = add("packages/nt_widget/assets/fonts/Geist-Variable.ttf", inter);
    m_monoFont = add("packages/nt_widget/assets/fonts/GeistMono-Variable.ttf", "monospace");
}

QString OfficialPack::assetPath(const QString &relativePath) const {
    if (!available() || !safeRelative(relativePath))
        return {};
    const QString path = m_dir + '/' + relativePath;
    return QFileInfo::exists(path) ? path : QString();
}

bool OfficialPack::hasAsset(const QString &relativePath) const { return !assetPath(relativePath).isEmpty(); }

QUrl OfficialPack::asset(const QString &relativePath) const {
    const QString path = assetPath(relativePath);
    return path.isEmpty() ? QUrl() : QUrl::fromLocalFile(path);
}

QJsonObject OfficialPack::modelConfig(const QString &modelId) const {
    for (const QJsonValue &v : m_whiteList)
        if (v.toObject().value("id").toString().compare(modelId, Qt::CaseInsensitive) == 0)
            return v.toObject();
    return {};
}

QJsonArray OfficialPack::modelSkus(const QString &modelId) const {
    QJsonArray out;
    for (const QJsonValue &v : m_skus)
        if (v.toObject().value("deviceSpu").toObject().value("modelId").toString().compare(modelId, Qt::CaseInsensitive) == 0)
            out.append(v);
    return out;
}

QUrl OfficialPack::productImage(const QString &modelId, const QString &colorId, const QString &kind) {
    QJsonObject sku;
    for (const QJsonValue &v : modelSkus(modelId))
        if (sku.isEmpty() || v.toObject().value("colorId").toString() == colorId)
            sku = v.toObject();
    const QUrl url(sku.value(kind + "ImageUrl").toString());
    if (!url.isValid() || url.scheme() != "https")
        return {};
    // Kept outside the versioned folders, so a new import doesn't refetch.
    const QString dir = m_cacheRoot + "/images";
    const QString file = dir + '/' + QCryptographicHash::hash(url.toEncoded(), QCryptographicHash::Sha1).toHex() + ".png";
    if (QFileInfo::exists(file))
        return QUrl::fromLocalFile(file);
    if (m_downloading.contains(file))
        return {};
    m_downloading.insert(file, true);
    if (!m_net)
        m_net = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("NothingX-Desktop"));
    QNetworkReply *reply = m_net->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, dir, file] {
        reply->deleteLater();
        m_downloading.remove(file);
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError || data.isEmpty())
            return;
        QDir().mkpath(dir);
        QSaveFile out(file);
        if (out.open(QIODevice::WriteOnly) && out.write(data) == data.size() && out.commit())
            emit imageReady();
    });
    return {};
}

bool OfficialPack::unpack(const QString &file, const QString &cacheRoot, QString *version, QString *error) {
    auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };
    const QString name = QFileInfo(file).fileName();
    if (!QFileInfo(file).isFile())
        return fail(tr("%1 doesn't exist.").arg(file));
    QDir().mkpath(cacheRoot);
    QTemporaryDir work(cacheRoot + "/import-XXXXXX");
    if (!work.isValid())
        return fail(tr("Couldn't write to %1.").arg(cacheRoot));

    // An .xapk wraps the base APK (named after the package) and split APKs;
    // its manifest.json carries the version. A plain .apk is the base itself.
    QString v = versionFromName(file);
    QString apk = file;
    if (extract(file, work.path(), {"manifest.json"})) {
        const QJsonObject manifest = readJson(work.filePath("manifest.json")).object();
        const QString package = manifest.value("package_name").toString("com.nothing.smartcenter");
        if (manifest.contains("version_name"))
            v = manifest.value("version_name").toString();
        if (!extract(file, work.path(), {package + ".apk"}))
            return fail(tr("%1 doesn't contain the Nothing X app.").arg(name));
        apk = work.filePath(package + ".apk");
    }
    if (v.isEmpty() || !safeRelative(v) || v.contains('/'))
        v = "0";

    const QString unpacked = work.filePath("apk");
    QDir().mkpath(unpacked);
    // tar fails when a pattern matches nothing; the config is what matters.
    extract(apk, unpacked, Wanted, Skipped);
    const QString flutter = unpacked + "/assets/flutter_assets";
    if (!QFileInfo::exists(flutter + "/assets/config/ear_white_list.json"))
        return fail(tr("%1 isn't the official Nothing X app.").arg(name));

    // Flutter assets keep their asset keys ("assets/images/ear/x.svg"); the
    // native Android ones go under native/. Built next to the final folder,
    // then swapped in, so a failed import never breaks a working one.
    const QString target = cacheRoot + '/' + v;
    const QString partial = target + ".partial";
    QDir(partial).removeRecursively();
    if (!QDir().rename(flutter, partial))
        return fail(tr("Couldn't write to %1.").arg(cacheRoot));
    QDir().mkpath(partial + "/native");
    for (const char *native : {"lottie", "fonts"})
        QDir().rename(unpacked + "/assets/" + native, partial + "/native/" + native);
    QDir().rename(unpacked + "/res/drawable", partial + "/native/drawable");

    QSaveFile meta(partial + "/pack.json");
    if (!meta.open(QIODevice::WriteOnly))
        return fail(tr("Couldn't write to %1.").arg(partial));
    meta.write(QJsonDocument(QJsonObject{{"version", v}, {"source", name}}).toJson());
    meta.commit();
    QDir(target).removeRecursively();
    if (!QDir().rename(partial, target))
        return fail(tr("Couldn't finish unpacking into %1.").arg(target));

    // Only the newest version is kept, to save space.
    const QDir root(cacheRoot);
    static const QRegularExpression versionName(R"(^\d+(\.\d+)*$)");
    for (const QString &dir : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (dir != v && versionName.match(dir).hasMatch() && compareVersions(dir, v) <= 0)
            QDir(root.filePath(dir)).removeRecursively();
    if (version)
        *version = v;
    return true;
}

void OfficialPack::importFile(const QString &pathOrUrl) {
    if (m_importing)
        return;
    // Dropped files arrive as file:// URLs.
    const QString path = pathOrUrl.startsWith("file:") ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
    m_importing = true;
    m_lastError.clear();
    emit importingChanged();
    QPointer<OfficialPack> self(this);
    const QString root = m_cacheRoot;
    QThread *worker = QThread::create([self, path, root] {
        QString version, error;
        const bool ok = unpack(path, root, &version, &error);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [self, ok, version, error] {
            if (!self)
                return;
            self->m_importing = false;
            self->m_lastError = ok ? QString() : error;
            if (ok)
                self->load();
            emit self->importingChanged();
            emit self->importFinished(ok, ok ? tr("Imported Nothing X %1.").arg(version) : error);
        });
    });
    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start(QThread::LowPriority);
}

QStringList OfficialPack::knownPlaces() {
    QStringList dirs;
    for (auto location : {QStandardPaths::DownloadLocation, QStandardPaths::DesktopLocation,
                          QStandardPaths::DocumentsLocation, QStandardPaths::HomeLocation})
        dirs << QStandardPaths::writableLocation(location);
    dirs.removeDuplicates();
    return dirs;
}

bool OfficialPack::importFromKnownPlaces() {
    QString best, bestVersion;
    static const QRegularExpression name(R"((nothing[ _+.-]?x|smartcenter).*\.(x?apk)$)",
                                         QRegularExpression::CaseInsensitiveOption);
    for (const QString &dir : knownPlaces()) {
        QDirIterator it(dir, {"*.xapk", "*.apk"}, QDir::Files);
        while (it.hasNext()) {
            const QString file = it.next();
            if (!name.match(QFileInfo(file).fileName()).hasMatch())
                continue;
            const QString v = versionFromName(file);
            if (best.isEmpty() || compareVersions(v, bestVersion) > 0) {
                best = file;
                bestVersion = v;
            }
        }
    }
    if (best.isEmpty() || (available() && compareVersions(bestVersion, m_version) <= 0))
        return false;
    importFile(best);
    return true;
}
