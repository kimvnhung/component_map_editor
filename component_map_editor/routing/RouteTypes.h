#ifndef ROUTETYPES_H
#define ROUTETYPES_H

#include <QList>
#include <QPointF>

#include "models/ConnectionModel.h"

class ComponentModel;

struct ConnectionRouteMeta {
    ConnectionModel::Side sourceSide = ConnectionModel::SideAuto;
    ConnectionModel::Side targetSide = ConnectionModel::SideAuto;
    qreal sourceTangentOffset = 0.0;
    qreal targetTangentOffset = 0.0;
};

struct RouteRequest {
    ConnectionModel *connection = nullptr;
    ComponentModel *sourceComponent = nullptr;
    ComponentModel *targetComponent = nullptr;
    const ConnectionRouteMeta *routeMeta = nullptr;
};

struct RoutingContext {
    const QList<ComponentModel *> *components = nullptr;
};

#endif // ROUTETYPES_H
