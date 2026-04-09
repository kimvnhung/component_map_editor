#include "RoutingHelpers.h"

#include "models/ComponentModel.h"

#include <QtMath>

namespace RoutingHelpers {

namespace {

QPointF centerPoint(ComponentModel *component)
{
    return QPointF(component->x(), component->y());
}

ConnectionModel::Side autoSideForComponents(ComponentModel *sourceComponent,
                                             ComponentModel *targetComponent,
                                             bool isSource)
{
    const QPointF sourceCenter = centerPoint(sourceComponent);
    const QPointF targetCenter = centerPoint(targetComponent);
    const qreal dx = targetCenter.x() - sourceCenter.x();
    const qreal dy = targetCenter.y() - sourceCenter.y();

    if (qAbs(dx) >= qAbs(dy)) {
        if (dx >= 0.0)
            return isSource ? ConnectionModel::SideRight : ConnectionModel::SideLeft;
        return isSource ? ConnectionModel::SideLeft : ConnectionModel::SideRight;
    }

    if (dy >= 0.0)
        return isSource ? ConnectionModel::SideBottom : ConnectionModel::SideTop;
    return isSource ? ConnectionModel::SideTop : ConnectionModel::SideBottom;
}

} // anonymous namespace

QRectF componentWorldRect(ComponentModel *component)
{
    return QRectF(component->x() - component->width() / 2.0,
                  component->y() - component->height() / 2.0,
                  component->width(),
                  component->height());
}

bool isHorizontalSide(ConnectionModel::Side side)
{
    return side == ConnectionModel::SideLeft || side == ConnectionModel::SideRight;
}

ConnectionModel::Side resolvedSide(ConnectionModel::Side preferredSide,
                                   ComponentModel *sourceComponent,
                                   ComponentModel *targetComponent,
                                   bool isSource)
{
    if (preferredSide != ConnectionModel::SideAuto)
        return preferredSide;

    return autoSideForComponents(sourceComponent, targetComponent, isSource);
}

QPointF anchorPointForSide(ComponentModel *component, ConnectionModel::Side side)
{
    const QRectF rect = componentWorldRect(component);
    switch (side) {
    case ConnectionModel::SideTop:
        return QPointF(rect.center().x(), rect.top());
    case ConnectionModel::SideRight:
        return QPointF(rect.right(), rect.center().y());
    case ConnectionModel::SideBottom:
        return QPointF(rect.center().x(), rect.bottom());
    case ConnectionModel::SideLeft:
    case ConnectionModel::SideAuto:
        return QPointF(rect.left(), rect.center().y());
    }

    return rect.center();
}

QPointF offsetPointForSide(const QPointF &point, ConnectionModel::Side side, qreal distance)
{
    switch (side) {
    case ConnectionModel::SideTop:
        return QPointF(point.x(), point.y() - distance);
    case ConnectionModel::SideRight:
        return QPointF(point.x() + distance, point.y());
    case ConnectionModel::SideBottom:
        return QPointF(point.x(), point.y() + distance);
    case ConnectionModel::SideLeft:
    case ConnectionModel::SideAuto:
        return QPointF(point.x() - distance, point.y());
    }

    return point;
}

QPointF applyTangentOffsetForSide(const QPointF &point,
                                   ConnectionModel::Side side,
                                   qreal tangentOffset)
{
    if (qFuzzyIsNull(tangentOffset))
        return point;

    switch (side) {
    case ConnectionModel::SideTop:
    case ConnectionModel::SideBottom:
        return QPointF(point.x() + tangentOffset, point.y());
    case ConnectionModel::SideLeft:
    case ConnectionModel::SideRight:
        return QPointF(point.x(), point.y() + tangentOffset);
    case ConnectionModel::SideAuto:
        return point;
    }

    return point;
}

QVector<QPointF> simplifyPolyline(const QVector<QPointF> &points)
{
    QVector<QPointF> simplified;
    simplified.reserve(points.size());

    for (const QPointF &point : points) {
        if (!simplified.isEmpty() && simplified.last() == point)
            continue;
        simplified.append(point);
    }

    bool removed = true;
    while (removed && simplified.size() >= 3) {
        removed = false;
        for (int i = 1; i < simplified.size() - 1; ++i) {
            const QPointF &a = simplified.at(i - 1);
            const QPointF &b = simplified.at(i);
            const QPointF &c = simplified.at(i + 1);
            if ((qFuzzyCompare(a.x(), b.x()) && qFuzzyCompare(b.x(), c.x()))
                || (qFuzzyCompare(a.y(), b.y()) && qFuzzyCompare(b.y(), c.y()))) {
                simplified.removeAt(i);
                removed = true;
                break;
            }
        }
    }

    return simplified;
}

QRectF polylineBounds(const QVector<QPointF> &points)
{
    if (points.isEmpty())
        return QRectF();

    qreal minX = points.first().x();
    qreal maxX = minX;
    qreal minY = points.first().y();
    qreal maxY = minY;
    for (const QPointF &point : points) {
        minX = qMin(minX, point.x());
        maxX = qMax(maxX, point.x());
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }

    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

QHash<const ConnectionModel *, ConnectionRouteMeta>
buildConnectionRouteMeta(const QList<ConnectionModel *> &connections,
                         const QHash<QString, ComponentModel *> &componentById)
{
    struct SideBucket {
        QVector<ConnectionModel *> outgoing;
        QVector<ConnectionModel *> incoming;
    };

    QHash<const ConnectionModel *, ConnectionRouteMeta> routeMetaByConnection;
    routeMetaByConnection.reserve(connections.size());
    QHash<QString, SideBucket> buckets;

    for (ConnectionModel *connection : connections) {
        if (!connection)
            continue;

        ComponentModel *src = componentById.value(connection->sourceId(), nullptr);
        ComponentModel *tgt = componentById.value(connection->targetId(), nullptr);
        if (!src || !tgt)
            continue;

        ConnectionRouteMeta meta;
        meta.sourceSide = resolvedSide(connection->sourceSide(), src, tgt, true);
        meta.targetSide = resolvedSide(connection->targetSide(), src, tgt, false);
        routeMetaByConnection.insert(connection, meta);

        const QString sourceKey = src->id() + QStringLiteral("|") + QString::number(int(meta.sourceSide));
        const QString targetKey = tgt->id() + QStringLiteral("|") + QString::number(int(meta.targetSide));
        buckets[sourceKey].outgoing.append(connection);
        buckets[targetKey].incoming.append(connection);
    }

    constexpr qreal flowBand = 7.0;
    for (auto it = buckets.begin(); it != buckets.end(); ++it) {
        SideBucket &bucket = it.value();
        if (bucket.outgoing.isEmpty() || bucket.incoming.isEmpty())
            continue;

        // Mixed flow only: keep same-type connections on the exact same edge
        // point, and separate only by direction class (outgoing vs incoming).
        for (ConnectionModel *connection : bucket.outgoing) {
            auto metaIt = routeMetaByConnection.find(connection);
            if (metaIt == routeMetaByConnection.end())
                continue;

            metaIt->sourceTangentOffset = -flowBand;
        }

        for (ConnectionModel *connection : bucket.incoming) {
            auto metaIt = routeMetaByConnection.find(connection);
            if (metaIt == routeMetaByConnection.end())
                continue;

            metaIt->targetTangentOffset = flowBand;
        }
    }

    return routeMetaByConnection;
}

} // namespace RoutingHelpers
