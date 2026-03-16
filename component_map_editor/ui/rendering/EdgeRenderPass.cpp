#include "EdgeRenderPass.h"

#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "routing/RouteTypes.h"
#include "routing/RoutingEngine.h"
#include "routing/RoutingHelpers.h"

#include <QColor>
#include <QHash>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGNode>
#include <QVector>

#include <cmath>

namespace EdgeRenderPass {
namespace {

void clearNode(QSGGeometryNode *n)
{
    if (!n)
        return;
    if (n->geometry()->vertexCount() > 0) {
        n->geometry()->allocate(0);
        n->markDirty(QSGNode::DirtyGeometry);
    }
}

void appendArrowTriangle(QSGGeometry::ColoredPoint2D *verts,
                         int &idx,
                         const QPointF &tip,
                         const QPointF &from,
                         const QColor &color,
                         qreal length,
                         qreal width)
{
    const QPointF dir = tip - from;
    const qreal lenSq = dir.x() * dir.x() + dir.y() * dir.y();
    if (lenSq <= 0.0001)
        return;

    const qreal invLen = 1.0 / std::sqrt(lenSq);
    const QPointF unit(dir.x() * invLen, dir.y() * invLen);
    const QPointF normal(-unit.y(), unit.x());

    const QPointF baseCenter = tip - unit * length;
    const QPointF left = baseCenter + normal * width;
    const QPointF right = baseCenter - normal * width;

    const unsigned char r = color.red();
    const unsigned char g = color.green();
    const unsigned char b = color.blue();
    const unsigned char a = color.alpha();

    verts[idx++].set(float(tip.x()), float(tip.y()), r, g, b, a);
    verts[idx++].set(float(left.x()), float(left.y()), r, g, b, a);
    verts[idx++].set(float(right.x()), float(right.y()), r, g, b, a);
}

} // namespace

void updateEdgesGeometry(QObject *graph,
                         RoutingEngine *routingEngine,
                         QObject *selectedConnection,
                         bool renderEdges,
                         bool lodSimpleEdges,
                         QSGGeometryNode *normalEdgesGeomNode,
                         QSGGeometryNode *selectedEdgesGeomNode,
                         QSGGeometryNode *normalArrowsGeomNode,
                         QSGGeometryNode *selectedArrowsGeomNode)
{
    if (!normalEdgesGeomNode || !selectedEdgesGeomNode || !normalArrowsGeomNode || !selectedArrowsGeomNode)
        return;

    if (!renderEdges) {
        clearNode(normalEdgesGeomNode);
        clearNode(selectedEdgesGeomNode);
        clearNode(normalArrowsGeomNode);
        clearNode(selectedArrowsGeomNode);
        return;
    }

    auto *graphModel = qobject_cast<GraphModel *>(graph);
    if (!graphModel) {
        clearNode(normalEdgesGeomNode);
        clearNode(selectedEdgesGeomNode);
        clearNode(normalArrowsGeomNode);
        clearNode(selectedArrowsGeomNode);
        return;
    }

    const auto &components = graphModel->componentList();
    const auto &connections = graphModel->connectionList();

    QHash<QString, ComponentModel *> componentById;
    componentById.reserve(components.size());
    for (ComponentModel *c : components)
        componentById.insert(c->id(), c);

    const QHash<const ConnectionModel *, ConnectionRouteMeta> routeMetaByConnection =
        RoutingHelpers::buildConnectionRouteMeta(connections, componentById);

    QVector<QVector<QPointF>> cachedRoutes;
    cachedRoutes.reserve(connections.size());
    for (ConnectionModel *conn : connections) {
        ComponentModel *src = componentById.value(conn->sourceId(), nullptr);
        ComponentModel *tgt = componentById.value(conn->targetId(), nullptr);
        if (!src || !tgt) {
            cachedRoutes.append(QVector<QPointF>());
            continue;
        }

        const auto metaIt = routeMetaByConnection.constFind(conn);
        const ConnectionRouteMeta *routeMeta = (metaIt != routeMetaByConnection.constEnd())
            ? &metaIt.value()
            : nullptr;
        QVector<QPointF> route;
        if (routingEngine) {
            route = routingEngine->compute(RouteRequest { conn, src, tgt, routeMeta },
                                           RoutingContext { &components });
        }
        cachedRoutes.append(route);
    }

    int normalCount = 0, selectedCount = 0;
    int normalArrowCount = 0, selectedArrowCount = 0;
    for (int ci = 0; ci < connections.size(); ++ci) {
        const QVector<QPointF> &route = cachedRoutes.at(ci);
        if (route.isEmpty())
            continue;

        const int segmentCount = qMax(0, route.size() - 1);
        if (static_cast<QObject *>(connections.at(ci)) == selectedConnection)
            selectedCount += segmentCount;
        else
            normalCount += segmentCount;

        if (route.size() >= 2) {
            if (static_cast<QObject *>(connections.at(ci)) == selectedConnection)
                ++selectedArrowCount;
            else
                ++normalArrowCount;
        }
    }

    if (normalEdgesGeomNode->geometry()->vertexCount() != normalCount * 2)
        normalEdgesGeomNode->geometry()->allocate(normalCount * 2);
    if (selectedEdgesGeomNode->geometry()->vertexCount() != selectedCount * 2)
        selectedEdgesGeomNode->geometry()->allocate(selectedCount * 2);
    if (normalArrowsGeomNode->geometry()->vertexCount() != normalArrowCount * 3)
        normalArrowsGeomNode->geometry()->allocate(normalArrowCount * 3);
    if (selectedArrowsGeomNode->geometry()->vertexCount() != selectedArrowCount * 3)
        selectedArrowsGeomNode->geometry()->allocate(selectedArrowCount * 3);

    auto *normalV = normalEdgesGeomNode->geometry()->vertexDataAsPoint2D();
    auto *selectedV = selectedEdgesGeomNode->geometry()->vertexDataAsPoint2D();
    auto *normalArrowV = normalArrowsGeomNode->geometry()->vertexDataAsColoredPoint2D();
    auto *selectedArrowV = selectedArrowsGeomNode->geometry()->vertexDataAsColoredPoint2D();
    int nIdx = 0, sIdx = 0;
    int nArrowIdx = 0, sArrowIdx = 0;

    const qreal arrowLength = lodSimpleEdges ? 8.0 : 12.0;
    const qreal arrowWidth = lodSimpleEdges ? 3.5 : 5.0;
    const QColor normalColor = lodSimpleEdges
        ? QColor(QStringLiteral("#546e7a"))
        : QColor(QStringLiteral("#607d8b"));
    const QColor selectedColor = lodSimpleEdges
        ? QColor(QStringLiteral("#546e7a"))
        : QColor(QStringLiteral("#ff5722"));

    for (int ci = 0; ci < connections.size(); ++ci) {
        const QVector<QPointF> &route = cachedRoutes.at(ci);
        if (route.size() < 2)
            continue;

        ConnectionModel *conn = connections.at(ci);

        if (static_cast<QObject *>(conn) == selectedConnection) {
            for (int i = 1; i < route.size(); ++i) {
                selectedV[sIdx++].set(float(route.at(i - 1).x()), float(route.at(i - 1).y()));
                selectedV[sIdx++].set(float(route.at(i).x()), float(route.at(i).y()));
            }

            appendArrowTriangle(selectedArrowV,
                                sArrowIdx,
                                route.last(),
                                route.at(route.size() - 2),
                                selectedColor,
                                arrowLength,
                                arrowWidth);
        } else {
            for (int i = 1; i < route.size(); ++i) {
                normalV[nIdx++].set(float(route.at(i - 1).x()), float(route.at(i - 1).y()));
                normalV[nIdx++].set(float(route.at(i).x()), float(route.at(i).y()));
            }

            appendArrowTriangle(normalArrowV,
                                nArrowIdx,
                                route.last(),
                                route.at(route.size() - 2),
                                normalColor,
                                arrowLength,
                                arrowWidth);
        }
    }

    normalEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
    selectedEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
    normalArrowsGeomNode->markDirty(QSGNode::DirtyGeometry);
    selectedArrowsGeomNode->markDirty(QSGNode::DirtyGeometry);
}

void updateTempEdgeGeometry(bool tempConnectionDragging,
                            const QPointF &tempStart,
                            const QPointF &tempEnd,
                            QSGGeometryNode *tempEdgeGeomNode)
{
    if (!tempEdgeGeomNode)
        return;

    auto *geom = tempEdgeGeomNode->geometry();

    if (!tempConnectionDragging) {
        if (geom->vertexCount() > 0) {
            geom->allocate(0);
            tempEdgeGeomNode->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    if (geom->vertexCount() != 2)
        geom->allocate(2);

    auto *v = geom->vertexDataAsPoint2D();
    v[0].set(float(tempStart.x()), float(tempStart.y()));
    v[1].set(float(tempEnd.x()), float(tempEnd.y()));
    tempEdgeGeomNode->markDirty(QSGNode::DirtyGeometry);
}

} // namespace EdgeRenderPass
