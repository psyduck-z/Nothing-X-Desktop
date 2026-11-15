#pragma once

#include <QQuickImageProvider>

class OfficialPack;

// Serves the official app's artwork from the imported pack:
// image://official/<rrggbb>/<asset path> tints a one-colour icon, the way the
// official Flutter app colours its SVGs for light and dark mode;
// image://official/-/<asset path> draws it as it is (photos, illustrations).
class OfficialImageProvider : public QQuickImageProvider {
public:
    explicit OfficialImageProvider(OfficialPack *pack);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    // Renders an SVG or bitmap file at the requested size, keeping its aspect
    // ratio; tints every pixel when a valid colour is given.
    static QImage render(const QString &file, const QSize &requestedSize, const QColor &tint);

private:
    OfficialPack *m_pack;
};
