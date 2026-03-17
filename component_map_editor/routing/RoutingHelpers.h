#ifndef ROUTINGHELPERS_H
#define ROUTINGHELPERS_H

#include <QHash>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QVector>

#include "RouteTypes.h"
#include "models/ConnectionModel.h"

class ComponentModel;

// Shared routing utility functions used by routing strategies and the viewport.
// All functions live in the RoutingHelpers namespace to prevent ODR conflicts.
namespace RoutingHelpers {

QRectF componentWorldRect(ComponentModel *component);
bool isHorizontalSide(ConnectionModel::Side side);

ConnectionModel::Side resolvedSide(ConnectionModel::Side preferredSide,
                                   ComponentModel *sourceComponent,
                                   ComponentModel *targetComponent,
                                   bool isSource);

QPointF anchorPointForSide(ComponentModel *component, ConnectionModel::Side side);

QPointF offsetPointForSide(const QPointF &point,
                            ConnectionModel::Side side,
                            qreal distance);

QPointF applyTangentOffsetForSide(const QPointF &point,
                                   ConnectionModel::Side side,
                                   qreal tangentOffset);

QVector<QPointF> simplifyPolyline(const QVector<QPointF> &points);

QRectF polylineBounds(const QVector<QPointF> &points);

// Computes per-connection tangent offsets for all connections at once.
// Call this once before routing individual connections to get layout-consistent
// source/target tangent offsets that separate bidirectional edges visually.
QHash<const ConnectionModel *, ConnectionRouteMeta>
buildConnectionRouteMeta(const QList<ConnectionModel *> &connections,
                         const QHash<QString, ComponentModel *> &componentById);

} // namespace RoutingHelpers

#endif // ROUTINGHELPERS_H
