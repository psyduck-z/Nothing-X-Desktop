#include "officialimageprovider.h"
#include "officialpack.h"

#include <QImageReader>
#include <QPainter>
#include <QSvgRenderer>

OfficialImageProvider::OfficialImageProvider(OfficialPack *pack)
    : QQuickImageProvider(QQuickImageProvider::Image), m_pack(pack) {}

QImage OfficialImageProvider::render(const QString &file, const QSize &requestedSize, const QColor &tint) {
    QImage image;
    if (file.endsWith(".svg", Qt::CaseInsensitive)) {
        QSvgRenderer svg(file);
        if (!svg.isValid())
            return {};
        QSize natural = svg.defaultSize().isEmpty() ? QSize(24, 24) : svg.defaultSize();
        QSize out = natural;
        if (requestedSize.width() > 0 || requestedSize.height() > 0)
            out = natural.scaled(requestedSize.width() > 0 ? requestedSize.width() : 1 << 14,
                                 requestedSize.height() > 0 ? requestedSize.height() : 1 << 14, Qt::KeepAspectRatio);
        image = QImage(out, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing);
        svg.render(&p);
    } else {
        QImageReader reader(file);
        if (requestedSize.width() > 0 || requestedSize.height() > 0) {
            const QSize natural = reader.size();
            if (natural.isValid())
                reader.setScaledSize(natural.scaled(requestedSize.width() > 0 ? requestedSize.width() : 1 << 14,
                                                    requestedSize.height() > 0 ? requestedSize.height() : 1 << 14,
                                                    Qt::KeepAspectRatio));
        }
        image = reader.read().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    if (tint.isValid() && !image.isNull()) {
        QPainter p(&image);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(image.rect(), tint);
    }
    return image;
}

QImage OfficialImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    const QString hex = id.section('/', 0, 0);
    const QString path = id.section('/', 1);
    const QColor tint = hex == "-" ? QColor() : QColor("#" + hex);
    QImage image = render(m_pack->assetPath(path), requestedSize, tint);
    if (size)
        *size = image.size();
    return image;
}
