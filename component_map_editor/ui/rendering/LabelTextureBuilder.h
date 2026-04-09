#ifndef LABELTEXTUREBUILDER_H
#define LABELTEXTUREBUILDER_H

#include <QImage>
#include <QString>

class ComponentModel;
class QFont;

namespace LabelTextureBuilder {

QString makeLabelCacheKey(const ComponentModel *component,
                          qreal devicePixelRatio,
                          int availableWidth,
                          int availableHeight);

QImage renderLabelImage(const ComponentModel *component,
                        int availableWidth,
                        int availableHeight,
                        qreal devicePixelRatio,
                        const QFont &titleFont,
                        const QFont &contentFont,
                        const QFont &iconFont);

} // namespace LabelTextureBuilder

#endif // LABELTEXTUREBUILDER_H
