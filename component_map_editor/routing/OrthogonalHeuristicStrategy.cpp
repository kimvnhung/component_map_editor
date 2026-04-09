#include "OrthogonalHeuristicStrategy.h"

#include "RoutingHelpers.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"

#include <QDebug>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QVector>

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

using namespace RoutingHelpers;

namespace {

bool axisAlignedSegmentIntersectsRectInterior(const QPointF &a,
                                              const QPointF &b,
                                              const QRectF &rect)
{
    constexpr qreal epsilon = 0.01;
    const QRectF innerRect = rect.adjusted(epsilon, epsilon, -epsilon, -epsilon);
    if (innerRect.isEmpty())
        return false;

    if (qFuzzyCompare(a.x(), b.x())) {
        const qreal x = a.x();
        if (x <= innerRect.left() || x >= innerRect.right())
            return false;

        const qreal minY = qMin(a.y(), b.y());
        const qreal maxY = qMax(a.y(), b.y());
        return maxY > innerRect.top() && minY < innerRect.bottom();
    }

    if (qFuzzyCompare(a.y(), b.y())) {
        const qreal y = a.y();
        if (y <= innerRect.top() || y >= innerRect.bottom())
            return false;

        const qreal minX = qMin(a.x(), b.x());
        const qreal maxX = qMax(a.x(), b.x());
        return maxX > innerRect.left() && minX < innerRect.right();
    }

    return true;
}

bool segmentUsesSideNormal(const QPointF &anchor,
                           const QPointF &other,
                           ConnectionModel::Side side)
{
    constexpr qreal epsilon = 0.0001;

    switch (side) {
    case ConnectionModel::SideTop:
        return qAbs(other.x() - anchor.x()) <= epsilon && other.y() < anchor.y() - epsilon;
    case ConnectionModel::SideRight:
        return qAbs(other.y() - anchor.y()) <= epsilon && other.x() > anchor.x() + epsilon;
    case ConnectionModel::SideBottom:
        return qAbs(other.x() - anchor.x()) <= epsilon && other.y() > anchor.y() + epsilon;
    case ConnectionModel::SideLeft:
        return qAbs(other.y() - anchor.y()) <= epsilon && other.x() < anchor.x() - epsilon;
    case ConnectionModel::SideAuto:
        return true;
    }

    return true;
}

bool routeIsValid(const QVector<QPointF> &points,
                  const QList<ComponentModel *> &components,
                  ComponentModel *sourceComponent,
                  ComponentModel *targetComponent,
                  ConnectionModel::Side sourceSide,
                  ConnectionModel::Side targetSide)
{
    if (points.size() < 2)
        return true;

    if (!segmentUsesSideNormal(points.first(), points.at(1), sourceSide))
        return false;
    if (!segmentUsesSideNormal(points.last(), points.at(points.size() - 2), targetSide))
        return false;

    for (int i = 1; i < points.size(); ++i) {
        const QPointF &a = points.at(i - 1);
        const QPointF &b = points.at(i);
        if (!qFuzzyCompare(a.x(), b.x()) && !qFuzzyCompare(a.y(), b.y()))
            return false;

        for (ComponentModel *component : components) {
            if (!component)
                continue;

            // Allow the route to start from the source boundary and end at the
            // target boundary. All other segments must avoid all component
            // interiors, including source and target.
            const bool isSourceBoundarySegment = (component == sourceComponent && i == 1);
            const bool isTargetBoundarySegment = (component == targetComponent && i == points.size() - 1);
            if (isSourceBoundarySegment || isTargetBoundarySegment)
                continue;

            if (axisAlignedSegmentIntersectsRectInterior(a, b, componentWorldRect(component)))
                return false;
        }
    }

    return true;
}

QVector<QPointF> makeLRoute(const QPointF &start, const QPointF &end, bool horizontalFirst)
{
    QVector<QPointF> points;
    points.reserve(3);
    points.append(start);
    points.append(horizontalFirst ? QPointF(end.x(), start.y())
                                  : QPointF(start.x(), end.y()));
    points.append(end);
    return simplifyPolyline(points);
}

QVector<QPointF> makeDoglegRoute(const QPointF &start,
                                 const QPointF &end,
                                 bool verticalCorridor,
                                 qreal corridor)
{
    QVector<QPointF> points;
    points.reserve(4);
    points.append(start);
    if (verticalCorridor) {
        points.append(QPointF(corridor, start.y()));
        points.append(QPointF(corridor, end.y()));
    } else {
        points.append(QPointF(start.x(), corridor));
        points.append(QPointF(end.x(), corridor));
    }
    points.append(end);
    return simplifyPolyline(points);
}

QVector<QPointF> makeGridAStarRoute(const QPointF &start,
                                    const QPointF &end,
                                    const QList<ComponentModel *> &components,
                                    ComponentModel *sourceComponent,
                                    ComponentModel *targetComponent)
{
    constexpr qreal baseCellSize = 24.0;
    constexpr qreal obstacleInflation = 10.0;
    constexpr qreal windowMargin = 180.0;
    constexpr int maxExpandedNodes = 8000;
    constexpr int maxGridCells = 12000;

    qreal minX = qMin(start.x(), end.x()) - windowMargin;
    qreal maxX = qMax(start.x(), end.x()) + windowMargin;
    qreal minY = qMin(start.y(), end.y()) - windowMargin;
    qreal maxY = qMax(start.y(), end.y()) + windowMargin;

    QVector<QRectF> obstacleRects;
    obstacleRects.reserve(components.size());
    for (ComponentModel *component : components) {
        if (!component || component == sourceComponent || component == targetComponent)
            continue;

        const QRectF inflated = componentWorldRect(component).adjusted(-obstacleInflation,
                                                                        -obstacleInflation,
                                                                        obstacleInflation,
                                                                        obstacleInflation);
        if (!inflated.intersects(QRectF(QPointF(minX, minY), QPointF(maxX, maxY))))
            continue;

        obstacleRects.append(inflated);
        minX = qMin(minX, inflated.left() - windowMargin * 0.2);
        maxX = qMax(maxX, inflated.right() + windowMargin * 0.2);
        minY = qMin(minY, inflated.top() - windowMargin * 0.2);
        maxY = qMax(maxY, inflated.bottom() + windowMargin * 0.2);
    }

    qreal cellSize = baseCellSize;
    int gridW = qMax(2, int(std::ceil((maxX - minX) / cellSize)) + 1);
    int gridH = qMax(2, int(std::ceil((maxY - minY) / cellSize)) + 1);
    while (gridW * gridH > maxGridCells) {
        cellSize *= 1.35;
        gridW = qMax(2, int(std::ceil((maxX - minX) / cellSize)) + 1);
        gridH = qMax(2, int(std::ceil((maxY - minY) / cellSize)) + 1);
    }

    auto toCellX = [&](qreal x) {
        return qBound(0, int(std::floor((x - minX) / cellSize)), gridW - 1);
    };
    auto toCellY = [&](qreal y) {
        return qBound(0, int(std::floor((y - minY) / cellSize)), gridH - 1);
    };
    auto toIndex = [&](int x, int y) {
        return y * gridW + x;
    };
    auto toCellCenter = [&](int x, int y) {
        return QPointF(minX + (x + 0.5) * cellSize,
                       minY + (y + 0.5) * cellSize);
    };

    QVector<uchar> occupied(gridW * gridH, 0);
    for (const QRectF &rect : obstacleRects) {
        const int left = toCellX(rect.left());
        const int right = toCellX(rect.right());
        const int top = toCellY(rect.top());
        const int bottom = toCellY(rect.bottom());
        for (int cy = top; cy <= bottom; ++cy) {
            for (int cx = left; cx <= right; ++cx)
                occupied[toIndex(cx, cy)] = 1;
        }
    }

    const int startX = toCellX(start.x());
    const int startY = toCellY(start.y());
    const int goalX = toCellX(end.x());
    const int goalY = toCellY(end.y());
    const int startIdx = toIndex(startX, startY);
    const int goalIdx = toIndex(goalX, goalY);

    occupied[startIdx] = 0;
    occupied[goalIdx] = 0;

    struct OpenNode {
        int f = 0;
        int idx = -1;
    };
    struct OpenNodeGreater {
        bool operator()(const OpenNode &a, const OpenNode &b) const
        {
            return a.f > b.f;
        }
    };

    auto heuristic = [&](int idx) {
        const int x = idx % gridW;
        const int y = idx / gridW;
        return qAbs(x - goalX) + qAbs(y - goalY);
    };

    const int totalCells = gridW * gridH;
    QVector<int> gScore(totalCells, std::numeric_limits<int>::max());
    QVector<int> parent(totalCells, -1);
    QVector<uchar> closed(totalCells, 0);
    std::priority_queue<OpenNode, std::vector<OpenNode>, OpenNodeGreater> open;

    gScore[startIdx] = 0;
    open.push(OpenNode { heuristic(startIdx), startIdx });

    int expanded = 0;
    bool found = false;

    while (!open.empty()) {
        const OpenNode current = open.top();
        open.pop();

        if (current.idx < 0 || current.idx >= totalCells)
            continue;
        if (closed[current.idx])
            continue;

        closed[current.idx] = 1;
        if (current.idx == goalIdx) {
            found = true;
            break;
        }

        ++expanded;
        if (expanded > maxExpandedNodes)
            break;

        const int cx = current.idx % gridW;
        const int cy = current.idx / gridW;
        const int neighborX[4] = { cx + 1, cx - 1, cx, cx };
        const int neighborY[4] = { cy, cy, cy + 1, cy - 1 };

        for (int i = 0; i < 4; ++i) {
            const int nx = neighborX[i];
            const int ny = neighborY[i];
            if (nx < 0 || nx >= gridW || ny < 0 || ny >= gridH)
                continue;

            const int nIdx = toIndex(nx, ny);
            if (occupied[nIdx] || closed[nIdx])
                continue;

            const int tentativeG = gScore[current.idx] + 1;
            if (tentativeG >= gScore[nIdx])
                continue;

            parent[nIdx] = current.idx;
            gScore[nIdx] = tentativeG;
            open.push(OpenNode { tentativeG + heuristic(nIdx), nIdx });
        }
    }

    if (!found && startIdx != goalIdx)
        return {};

    QVector<int> cellPath;
    cellPath.reserve(128);
    int cursor = goalIdx;
    cellPath.append(cursor);
    while (cursor != startIdx) {
        cursor = parent[cursor];
        if (cursor < 0)
            return {};
        cellPath.append(cursor);
    }
    std::reverse(cellPath.begin(), cellPath.end());

    // Build a world-space polyline that is strictly orthogonal (axis-aligned).
    // The A* grid search itself moves only in 4-neighbor fashion, so segments
    // between cell centers are axis-aligned. However, the exact start/end
    // points are not necessarily aligned with the first/last cell centers,
    // so we add at most one orthogonal "bridge" point at each end to avoid
    // creating diagonal segments that would fail validation.
    auto isAligned = [](const QPointF &a, const QPointF &b) -> bool {
        return qFuzzyIsNull(a.x() - b.x()) || qFuzzyIsNull(a.y() - b.y());
    };

    auto appendDistinct = [](QVector<QPointF> &vec, const QPointF &pt) {
        if (vec.isEmpty() || vec.last() != pt)
            vec.append(pt);
    };

    QVector<QPointF> worldPath;
    // Reserve a few extra slots for possible bridge points.
    worldPath.reserve(cellPath.size() + 4);

    // If there are no internal grid cells between start and goal, just create
    // a simple orthogonal (L-shaped) route between start and end.
    if (cellPath.size() <= 2) {
        appendDistinct(worldPath, start);
        if (!isAligned(start, end)) {
            // Use a single intermediate point to make an L-shape.
            QPointF mid(end.x(), start.y());
            appendDistinct(worldPath, mid);
        }
        appendDistinct(worldPath, end);
        return simplifyPolyline(worldPath);
    }

    // There is at least one internal cell. Connect start to the first cell
    // center via an orthogonal bridge point if necessary, then follow all
    // internal cell centers, and finally connect to end similarly.
    appendDistinct(worldPath, start);

    const int firstIdx = cellPath.at(1);
    QPointF firstCenter = toCellCenter(firstIdx % gridW, firstIdx / gridW);
    if (!isAligned(start, firstCenter)) {
        // Bridge horizontally first, then vertically.
        QPointF bridge(start.x(), firstCenter.y());
        appendDistinct(worldPath, bridge);
    }

    // Append all internal cell centers (from cellPath[1] to cellPath[size-2]).
    for (int i = 1; i < cellPath.size() - 1; ++i) {
        const int idx = cellPath.at(i);
        appendDistinct(worldPath, toCellCenter(idx % gridW, idx / gridW));
    }

    // Connect from the last cell center to the exact end point.
    const int lastIdx = cellPath.at(cellPath.size() - 2);
    QPointF lastCenter = toCellCenter(lastIdx % gridW, lastIdx / gridW);
    if (!isAligned(lastCenter, end)) {
        QPointF bridge(end.x(), lastCenter.y());
        appendDistinct(worldPath, bridge);
    }

    appendDistinct(worldPath, end);
    return simplifyPolyline(worldPath);
}

} // namespace

// ---------------------------------------------------------------------------
// OrthogonalHeuristicStrategy::compute
// ---------------------------------------------------------------------------

QVector<QPointF> OrthogonalHeuristicStrategy::compute(const RouteRequest &request,
                                                       const RoutingContext &context) const
{
    ConnectionModel *connection = request.connection;
    ComponentModel *sourceComponent = request.sourceComponent;
    ComponentModel *targetComponent = request.targetComponent;
    const ConnectionRouteMeta *routeMeta = request.routeMeta;
    const QList<ComponentModel *> &components = *context.components;

    struct RouteCandidate {
        QVector<QPointF> points;
        const char *stage;
    };

    auto tryRoute = [&](ConnectionModel::Side requestedSourceSide,
                        ConnectionModel::Side requestedTargetSide,
                        const char *attemptName,
                        QVector<QPointF> &acceptedRoute,
                        ConnectionModel::Side &resolvedSourceSide,
                        ConnectionModel::Side &resolvedTargetSide) -> bool {
        Q_UNUSED(attemptName)
        resolvedSourceSide = resolvedSide(requestedSourceSide,
                                          sourceComponent,
                                          targetComponent,
                                          true);
        resolvedTargetSide = resolvedSide(requestedTargetSide,
                                          sourceComponent,
                                          targetComponent,
                                          false);

        qreal sourceTangentOffset = 0.0;
        qreal targetTangentOffset = 0.0;
        if (routeMeta) {
            if (resolvedSourceSide == routeMeta->sourceSide)
                sourceTangentOffset = routeMeta->sourceTangentOffset;
            if (resolvedTargetSide == routeMeta->targetSide)
                targetTangentOffset = routeMeta->targetTangentOffset;
        }

        const QPointF start = applyTangentOffsetForSide(
            anchorPointForSide(sourceComponent, resolvedSourceSide),
            resolvedSourceSide,
            sourceTangentOffset);
        const QPointF end = applyTangentOffsetForSide(
            anchorPointForSide(targetComponent, resolvedTargetSide),
            resolvedTargetSide,
            targetTangentOffset);
        const QRectF sourceRect = componentWorldRect(sourceComponent);
        const QRectF targetRect = componentWorldRect(targetComponent);

        const qreal clearance = qMax<qreal>(24.0,
                                            qMin(sourceRect.width(), sourceRect.height()) * 0.25);

        QVector<RouteCandidate> candidates;
        candidates.reserve(20);

        candidates.append({ simplifyPolyline({ start, end }), "straight" });
        candidates.append({ makeLRoute(start, end, true), "l-horizontal-first" });
        candidates.append({ makeLRoute(start, end, false), "l-vertical-first" });

        const qreal leftCorridor = qMin(sourceRect.left(), targetRect.left()) - clearance;
        const qreal rightCorridor = qMax(sourceRect.right(), targetRect.right()) + clearance;
        const qreal topCorridor = qMin(sourceRect.top(), targetRect.top()) - clearance;
        const qreal bottomCorridor = qMax(sourceRect.bottom(), targetRect.bottom()) + clearance;

        const QPointF sourceOut = offsetPointForSide(start, resolvedSourceSide, clearance);
        const QPointF targetOut = offsetPointForSide(end, resolvedTargetSide, clearance);

        if (isHorizontalSide(resolvedSourceSide)) {
            candidates.append({ makeDoglegRoute(start, end, true, sourceOut.x()), "dogleg-source-corridor" });
            candidates.append({ simplifyPolyline({ start, sourceOut, end }), "source-clearance-kink" });
        } else {
            candidates.append({ makeDoglegRoute(start, end, false, sourceOut.y()), "dogleg-source-corridor" });
            candidates.append({ simplifyPolyline({ start, sourceOut, end }), "source-clearance-kink" });
        }

        if (isHorizontalSide(resolvedTargetSide)) {
            candidates.append({ makeDoglegRoute(start, end, true, targetOut.x()), "dogleg-target-corridor" });
            candidates.append({ simplifyPolyline({ start, targetOut, end }), "target-clearance-kink" });
        } else {
            candidates.append({ makeDoglegRoute(start, end, false, targetOut.y()), "dogleg-target-corridor" });
            candidates.append({ simplifyPolyline({ start, targetOut, end }), "target-clearance-kink" });
        }

        candidates.append({ makeDoglegRoute(start, end, true,
                                            resolvedSourceSide == ConnectionModel::SideLeft ? leftCorridor : rightCorridor),
                            "dogleg-source-preferred-outer" });
        candidates.append({ makeDoglegRoute(start, end, true,
                                            resolvedTargetSide == ConnectionModel::SideLeft ? leftCorridor : rightCorridor),
                            "dogleg-target-preferred-outer" });
        candidates.append({ makeDoglegRoute(start, end, false,
                                            resolvedSourceSide == ConnectionModel::SideTop ? topCorridor : bottomCorridor),
                            "dogleg-source-preferred-outer" });
        candidates.append({ makeDoglegRoute(start, end, false,
                                            resolvedTargetSide == ConnectionModel::SideTop ? topCorridor : bottomCorridor),
                            "dogleg-target-preferred-outer" });

        candidates.append({ makeDoglegRoute(start, end, true, leftCorridor), "dogleg-global-left" });
        candidates.append({ makeDoglegRoute(start, end, true, rightCorridor), "dogleg-global-right" });
        candidates.append({ makeDoglegRoute(start, end, false, topCorridor), "dogleg-global-top" });
        candidates.append({ makeDoglegRoute(start, end, false, bottomCorridor), "dogleg-global-bottom" });

        for (const RouteCandidate &candidate : candidates) {
            const QVector<QPointF> simplified = simplifyPolyline(candidate.points);
            if (simplified.size() < 2)
                continue;
            if (!routeIsValid(simplified,
                              components,
                              sourceComponent,
                              targetComponent,
                              resolvedSourceSide,
                              resolvedTargetSide)) {
                continue;
            }

            acceptedRoute = simplified;
            return true;
        }

        // Stage 2: obstacle-aware bounded local-grid A* fallback.
        const QVector<QPointF> stage2Route = makeGridAStarRoute(start,
                                                                end,
                                                                components,
                                                                sourceComponent,
                                                                targetComponent);
        if (stage2Route.size() >= 2
            && routeIsValid(stage2Route,
                            components,
                            sourceComponent,
                            targetComponent,
                            resolvedSourceSide,
                            resolvedTargetSide)) {
            acceptedRoute = stage2Route;
            return true;
        }

        return false;
    };

    QVector<QPointF> route;
    ConnectionModel::Side resolvedSource = ConnectionModel::SideAuto;
    ConnectionModel::Side resolvedTarget = ConnectionModel::SideAuto;

    if (tryRoute(connection->sourceSide(),
                 connection->targetSide(),
                 "requested",
                 route,
                 resolvedSource,
                 resolvedTarget)) {
        return route;
    }

    if (connection->sourceSide() != ConnectionModel::SideAuto
        && tryRoute(ConnectionModel::SideAuto,
                    connection->targetSide(),
                    "auto-source",
                    route,
                    resolvedSource,
                    resolvedTarget)) {
        return route;
    }

    if (connection->targetSide() != ConnectionModel::SideAuto
        && tryRoute(connection->sourceSide(),
                    ConnectionModel::SideAuto,
                    "auto-target",
                    route,
                    resolvedSource,
                    resolvedTarget)) {
        return route;
    }

    if (tryRoute(ConnectionModel::SideAuto,
                 ConnectionModel::SideAuto,
                 "auto-both",
                 route,
                 resolvedSource,
                 resolvedTarget)) {
        return route;
    }

    const ConnectionModel::Side fallbackSource = resolvedSide(connection->sourceSide(),
                                                              sourceComponent,
                                                              targetComponent,
                                                              true);
    const ConnectionModel::Side fallbackTarget = resolvedSide(connection->targetSide(),
                                                              sourceComponent,
                                                              targetComponent,
                                                              false);
    const qreal fallbackSourceOffset = (routeMeta && fallbackSource == routeMeta->sourceSide)
        ? routeMeta->sourceTangentOffset
        : 0.0;
    const qreal fallbackTargetOffset = (routeMeta && fallbackTarget == routeMeta->targetSide)
        ? routeMeta->targetTangentOffset
        : 0.0;

    const QPointF fallbackStart = applyTangentOffsetForSide(
        anchorPointForSide(sourceComponent, fallbackSource),
        fallbackSource,
        fallbackSourceOffset);
    const QPointF fallbackEnd = applyTangentOffsetForSide(
        anchorPointForSide(targetComponent, fallbackTarget),
        fallbackTarget,
        fallbackTargetOffset);

    qWarning().nospace()
        << "OrthogonalHeuristicStrategy: no valid route for connection '"
        << connection->id()
        << "' sourceSide=" << int(fallbackSource)
        << " targetSide=" << int(fallbackTarget)
        << "; using deterministic straight fallback.";
    return simplifyPolyline({ fallbackStart, fallbackEnd });
}
