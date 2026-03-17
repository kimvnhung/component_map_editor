#ifndef NODERENDERPASS_H
#define NODERENDERPASS_H

#include <QColor>
#include <QRectF>
#include <QVector>

class QSGGeometryNode;

namespace NodeRenderPass {

void updateNodeBodyGeometry(QSGGeometryNode *nodeFillGeomNode,
                            QSGGeometryNode *nodeOutlineGeomNode,
                            bool renderNodes,
                            bool lodHideNodeOutlines,
                            const QVector<QRectF> &worldRects,
                            const QVector<QColor> &fillColors,
                            const QVector<bool> &selectedFlags);

} // namespace NodeRenderPass

#endif // NODERENDERPASS_H
