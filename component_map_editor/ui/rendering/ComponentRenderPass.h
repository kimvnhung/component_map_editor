#ifndef COMPONENTRENDERPASS_H
#define COMPONENTRENDERPASS_H

#include <QColor>
#include <QRectF>
#include <QVector>

class QSGGeometryNode;

namespace ComponentRenderPass {

void updateComponentBodyGeometry(QSGGeometryNode *componentFillGeomNode,
                                  QSGGeometryNode *componentOutlineGeomNode,
                                  bool renderComponents,
                                  bool lodHideComponentOutlines,
                                  const QVector<QRectF> &worldRects,
                                  const QVector<QColor> &fillColors,
                                  const QVector<bool> &selectedFlags);

} // namespace ComponentRenderPass

#endif // COMPONENTRENDERPASS_H
