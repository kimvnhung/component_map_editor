#ifndef EDGERENDERPASS_H
#define EDGERENDERPASS_H

#include <QPointF>
#include <QSet>
#include <QString>

class QObject;
class QSGGeometryNode;
class RoutingEngine;

namespace EdgeRenderPass {

void updateEdgesGeometry(QObject *graph,
                         RoutingEngine *routingEngine,
                         QObject *selectedConnection,
                         const QSet<QString> &selectedConnectionIdSet,
                         bool renderEdges,
                         bool lodSimpleEdges,
                         QSGGeometryNode *normalEdgesGeomNode,
                         QSGGeometryNode *selectedEdgesGeomNode,
                         QSGGeometryNode *normalArrowsGeomNode,
                         QSGGeometryNode *selectedArrowsGeomNode);

void updateTempEdgeGeometry(bool tempConnectionDragging,
                            const QPointF &tempStart,
                            const QPointF &tempEnd,
                            QSGGeometryNode *tempEdgeGeomNode);

} // namespace EdgeRenderPass

#endif // EDGERENDERPASS_H
