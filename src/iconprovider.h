#pragma once

#include <QQuickImageProvider>

// Serves the SVG icons in :/icons recoloured on the fly:
// image://icon/<name>/<rrggbb>. The icons draw with currentColor, so one
// file covers dark mode, light mode and the accent.
class IconProvider : public QQuickImageProvider {
public:
    IconProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    static QImage render(const QString &name, const QColor &color, const QSize &size);
};
