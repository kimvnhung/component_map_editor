#ifndef CONNECTIONRENDERPASS_H
#define CONNECTIONRENDERPASS_H

#include <QPointF>
#include <QSet>
#include <QString>

class QObject;
class QSGGeometryNode;
class RoutingEngine;

namespace ConnectionRenderPass {

void updateConnectionsGeometry(QObject *graph,
                               RoutingEngine *routingEngine,
                               QObject *selectedConnection,
                               const QSet<QString> &selectedConnectionIdSet,
                               bool renderConnections,
                               bool lodSimpleConnections,
                               QSGGeometryNode *normalConnectionsGeomNode,
                               QSGGeometryNode *selectedConnectionsGeomNode,
                               QSGGeometryNode *normalArrowsGeomNode,
                               QSGGeometryNode *selectedArrowsGeomNode);

void updateTempConnectionGeometry(bool tempConnectionDragging,
                                  const QPointF &tempStart,
                                  const QPointF &tempEnd,
                                  QSGGeometryNode *tempConnectionGeomNode);

} // namespace ConnectionRenderPass

#endif // CONNECTIONRENDERPASS_H
