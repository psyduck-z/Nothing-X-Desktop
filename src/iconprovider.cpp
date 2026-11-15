#include "iconprovider.h"

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

IconProvider::IconProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage IconProvider::render(const QString &name, const QColor &color, const QSize &size) {
    QFile file(":/icons/" + name + ".svg");
    QByteArray svg;
    if (file.open(QIODevice::ReadOnly))
        svg = file.readAll();
    svg.replace("currentColor", color.name(QColor::HexRgb).toUtf8());

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QSvgRenderer renderer(svg);
    if (renderer.isValid()) {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter);
    }
    return image;
}

QImage IconProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    const QString name = id.section('/', 0, 0);
    const QString hex = id.section('/', 1, 1);
    const QColor color = hex.isEmpty() ? QColor(Qt::white) : QColor("#" + hex);
    const QSize out = requestedSize.isValid() && !requestedSize.isEmpty() ? requestedSize : QSize(48, 48);
    if (size)
        *size = out;
    return render(name, color, out);
}
