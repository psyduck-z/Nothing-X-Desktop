#pragma once

#include <QHash>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QUrl>

// The official Nothing X look and device data, taken from the user's own copy
// of the official Android app (an .xapk or .apk). None of it is in this
// repository: the fonts, icons, product images, animations and the per-model
// feature lists are unpacked from the user's file into a local cache, and the
// newest imported version wins. Importing a newer official app later brings
// its new devices, features and artwork along without a new desktop release.
class OfficialPack : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(QString version READ version NOTIFY changed)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY importingChanged)
    // Font families registered from the pack, or system fallbacks without it.
    Q_PROPERTY(QString bodyFont READ bodyFont NOTIFY changed)
    Q_PROPERTY(QString headlineFont READ headlineFont NOTIFY changed)
    Q_PROPERTY(QString dotFont READ dotFont NOTIFY changed)
    Q_PROPERTY(QString monoFont READ monoFont NOTIFY changed)

public:
    // Tests and the screenshots pass their own cache directory.
    explicit OfficialPack(const QString &cacheRoot = {}, QObject *parent = nullptr);

    bool available() const { return !m_dir.isEmpty(); }
    QString version() const { return m_version; }
    bool importing() const { return m_importing; }
    QString lastError() const { return m_lastError; }
    QString bodyFont() const { return m_bodyFont; }
    QString headlineFont() const { return m_headlineFont; }
    QString dotFont() const { return m_dotFont; }
    QString monoFont() const { return m_monoFont; }

    // A file from the official Flutter assets, as a URL QML can load, e.g.
    // asset("assets/images/ear/card_anc_on.svg"); empty when missing.
    Q_INVOKABLE QUrl asset(const QString &relativePath) const;
    Q_INVOKABLE bool hasAsset(const QString &relativePath) const;
    QString assetPath(const QString &relativePath) const;

    // The official feature configuration of a model ("B173" for the Ear (3)),
    // straight from the app's ear_white_list.json; empty when unknown.
    QJsonObject modelConfig(const QString &modelId) const;
    // The product a reported model id belongs to: the earbuds can report an id
    // (the Ear (3) says "B201") that the official list files under another
    // product as its "supportId" (B173). Unknown ids come back unchanged.
    QString productId(const QString &reportedId) const;
    // The model's entries in devices_info_list.json, one per colour.
    QJsonArray modelSkus(const QString &modelId) const;
    // A product photo of the model in a colour ("01", "02", ...), kind being
    // global, box, left, right, setting, control or guide. Like the official
    // app, these come from Nothing's image server; they are downloaded once,
    // and imageReady() fires when a photo that wasn't cached yet arrives.
    Q_INVOKABLE QUrl productImage(const QString &modelId, const QString &colorId, const QString &kind);

    // Starts importing in the background; importFinished() reports the result.
    Q_INVOKABLE void importFile(const QString &path);
    // Imports the newest official app found in the usual download places,
    // if it is newer than what is cached. Returns whether one was found.
    Q_INVOKABLE bool importFromKnownPlaces();
    static QStringList knownPlaces();
    static QString defaultCacheRoot();
    // Re-reads the cache (after a terminal import, for example).
    Q_INVOKABLE void reload() { load(); }

    // Synchronous import, used by the worker thread and by the tests.
    static bool unpack(const QString &file, const QString &cacheRoot, QString *version, QString *error);
    // "3.10.3" from "Nothing+X_3.10.3_APKPure.xapk" and the like.
    static QString versionFromName(const QString &fileName);
    static int compareVersions(const QString &a, const QString &b);

signals:
    void changed();
    void importingChanged();
    void importFinished(bool ok, const QString &message);
    void imageReady();

private:
    void load();
    void registerFonts();

    QString m_cacheRoot;
    QString m_dir;
    QString m_version;
    bool m_importing = false;
    QString m_lastError;
    QString m_bodyFont, m_headlineFont, m_dotFont, m_monoFont;
    QJsonArray m_whiteList;
    QJsonArray m_skus;
    QNetworkAccessManager *m_net = nullptr;
    QHash<QString, bool> m_downloading;
};
