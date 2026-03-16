#include "LabelTextureBuilder.h"

#include "FontAwesome.h"
#include "models/ComponentModel.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <Qt>

#include <cmath>

namespace LabelTextureBuilder {

QString makeLabelCacheKey(const ComponentModel *component,
                          qreal devicePixelRatio,
                          int availableWidth,
                          int availableHeight)
{
    if (!component)
        return QString();

    return component->title() + QStringLiteral("|")
        + QString::number(qHash(component->content())) + QStringLiteral("|")
        + component->icon() + QStringLiteral("|")
        + QString::number(int(std::round(component->width()))) + QStringLiteral("|")
        + QString::number(int(std::round(component->height()))) + QStringLiteral("|")
        + QString::number(devicePixelRatio, 'f', 2) + QStringLiteral("|")
        + QString::number(availableWidth) + QStringLiteral("|")
        + QString::number(availableHeight);
}

QImage renderLabelImage(const ComponentModel *component,
                        int availableWidth,
                        int availableHeight,
                        qreal devicePixelRatio,
                        const QFont &titleFont,
                        const QFont &contentFont,
                        const QFont &iconFont)
{
    QImage image(QSize(int(std::ceil(availableWidth * devicePixelRatio)),
                       int(std::ceil(availableHeight * devicePixelRatio))),
                 QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio);
    image.fill(Qt::transparent);

    if (!component)
        return image;

    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const int headerHeight = qBound(20, int(std::round(availableHeight * 0.30)), 28);
    painter.fillRect(QRectF(0.0, 0.0, availableWidth, headerHeight), QColor(0, 0, 0, 48));

    const QString iconName = component->icon().isEmpty() ? QStringLiteral("cube") : component->icon();
    const QString iconGlyph = FontAwesome::iconForCpp(iconName, FontAwesome::Solid);
    int textLeft = 6;
    if (!iconGlyph.isEmpty()) {
        painter.setFont(iconFont);
        painter.setPen(Qt::white);
        const QRectF iconRect(6.0, 0.0, 14.0, headerHeight);
        painter.drawText(iconRect, Qt::AlignCenter, iconGlyph);
        textLeft = 24;
    }

    QFontMetrics titleMetrics(titleFont);
    painter.setFont(titleFont);
    painter.setPen(Qt::white);
    const int titleAvail = qMax(8, availableWidth - textLeft - 6);
    const QString title = titleMetrics.elidedText(component->title(), Qt::ElideRight, titleAvail);
    painter.drawText(QRectF(textLeft, 0.0, titleAvail, headerHeight),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     title);

    const QString content = component->content().trimmed();
    if (!content.isEmpty()) {
        painter.setFont(contentFont);
        painter.setPen(QColor(QStringLiteral("#e8f1f6")));
        const QRectF contentRect(4.0,
                                 headerHeight + 2.0,
                                 availableWidth - 8.0,
                                 qMax(8.0, qreal(availableHeight - headerHeight - 4.0)));
        painter.drawText(contentRect,
                         Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap,
                         content);
    }

    painter.end();
    return image;
}

} // namespace LabelTextureBuilder
