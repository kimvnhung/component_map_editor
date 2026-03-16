#include "GraphViewportItem.h"

#include "ComponentModel.h"
#include "ConnectionModel.h"
#include "FontAwesome.h"
#include "GraphModel.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QElapsedTimer>
#include <QImage>
#include <QMatrix4x4>
#include <QPainter>
#include <QRect>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGSimpleTextureNode>
#include <QSGTransformNode>
#include <QSGTexture>
#include <QSGVertexColorMaterial>
#include <QSet>
#include <QTimer>
#include <QThread>
#include <queue>
#include <algorithm>
#include <cmath>
#include <limits>
#include <QtGlobal>
#include <QtMath>

namespace {

class LabelTextureNode final : public QSGSimpleTextureNode
{
public:
    QString cacheKey;
    qint64 textureBytes = 0;
};

// Cached FontAwesome icon font configuration for GraphViewportItem labels.
// This must be initialized from the GUI thread (e.g., during application startup
// or GraphViewportItem component completion) by calling initializeGraphViewportIconFont().
static QString g_iconFontFamilySolid;
static int g_iconFontWeightSolid = -1;
static bool g_iconFontInitialized = false;

// GUI-thread-only initialization helper. Calling this from the scene graph render
// thread is not safe, because FontAwesome lazy-loading may perform I/O and
// QFontDatabase operations. The render-thread code path only reads the cached
// values and never calls FontAwesome directly.
static void initializeGraphViewportIconFont()
{
    if (g_iconFontInitialized)
        return;

    g_iconFontFamilySolid = FontAwesome::familyForCpp(FontAwesome::Solid);
    g_iconFontWeightSolid = FontAwesome::weightForCpp(FontAwesome::Solid);
    g_iconFontInitialized = true;
}

QPointF centerPoint(ComponentModel *component)
{
    return QPointF(component->x(), component->y());
}

QPointF worldToView(qreal worldX, qreal worldY, qreal panX, qreal panY, qreal zoom)
{
    return QPointF(worldX * zoom + panX, worldY * zoom + panY);
}

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

QPointF offsetPointForSide(const QPointF &point,
                          ConnectionModel::Side side,
                          qreal distance)
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

    QVector<QPointF> worldPath;
    worldPath.reserve(cellPath.size() + 2);
    worldPath.append(start);
    for (int i = 1; i < cellPath.size() - 1; ++i) {
        const int idx = cellPath.at(i);
        worldPath.append(toCellCenter(idx % gridW, idx / gridW));
    }
    worldPath.append(end);
    return simplifyPolyline(worldPath);
}

QVector<QPointF> orthogonalRouteForConnection(ConnectionModel *connection,
                                              ComponentModel *sourceComponent,
                                              ComponentModel *targetComponent,
                                              const QList<ComponentModel *> &components)
{
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

        const QPointF start = anchorPointForSide(sourceComponent, resolvedSourceSide);
        const QPointF end = anchorPointForSide(targetComponent, resolvedTargetSide);
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
    const QPointF fallbackStart = anchorPointForSide(sourceComponent, fallbackSource);
    const QPointF fallbackEnd = anchorPointForSide(targetComponent, fallbackTarget);

    qWarning().nospace()
        << "Orthogonal router fallback: no valid route for connection '"
        << connection->id()
        << "' sourceSide=" << int(fallbackSource)
        << " targetSide=" << int(fallbackTarget)
        << "; using deterministic straight fallback.";
    return simplifyPolyline({ fallbackStart, fallbackEnd });
}

QSGGeometryNode *createLineNode(const QVector<QPointF> &segments,
                                const QColor &color,
                                float lineWidth)
{
    if (segments.isEmpty())
        return nullptr;

    auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(),
                                     segments.size());
    geometry->setDrawingMode(QSGGeometry::DrawLines);
    geometry->setLineWidth(lineWidth);

    auto *vertices = geometry->vertexDataAsPoint2D();
    int idx = 0;
    for (const QPointF &s : segments) {
        vertices[idx++].set(s.x(), s.y());
    }

    auto *material = new QSGFlatColorMaterial();
    material->setColor(color);

    auto *node = new QSGGeometryNode();
    node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial, true);
    node->setGeometry(geometry);
    node->setMaterial(material);
    return node;
}

// Creates a persistent geometry node with zero initial vertices.
// The caller owns none of the sub-objects; the node owns them via OwnsXxx flags.
QSGGeometryNode *createEmptyLineNode(const QColor &color, float lineWidth)
{
    auto *geom = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 0);
    geom->setDrawingMode(QSGGeometry::DrawLines);
    geom->setLineWidth(lineWidth);

    auto *mat = new QSGFlatColorMaterial();
    mat->setColor(color);

    auto *node = new QSGGeometryNode();
    node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial, true);
    node->setGeometry(geom);
    node->setMaterial(mat);
    return node;
}

QSGGeometryNode *createEmptyColoredNode()
{
    auto *geom = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
    geom->setDrawingMode(QSGGeometry::DrawTriangles);

    auto *material = new QSGVertexColorMaterial();

    auto *node = new QSGGeometryNode();
    node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial, true);
    node->setGeometry(geom);
    node->setMaterial(material);
    return node;
}

} // namespace

GraphViewportItem::GraphViewportItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    // Phase 2: viewport can render grid/edges with scene graph nodes.
    setFlag(ItemHasContents, true);
}

void GraphViewportItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (newGeometry.size() == oldGeometry.size())
        return;

    m_cameraDirty = true;
    m_nodeDirty = true;
    update();
}

QObject *GraphViewportItem::graph() const
{
    return m_graph;
}

void GraphViewportItem::setGraph(QObject *value)
{
    if (m_graph == value)
        return;

    if (m_graph) {
        QObject::disconnect(m_componentsChangedConn);
        QObject::disconnect(m_connectionsChangedConn);
    }
    clearComponentGeometryConnections();
    clearConnectionGeometryConnections();

    m_graph = value;
    m_graphRebuildScheduled = false;
    auto *graphModel = qobject_cast<GraphModel *>(m_graph);
    if (graphModel) {
        m_componentsChangedConn = connect(graphModel, &GraphModel::componentsChanged,
                                          this, &GraphViewportItem::requestGraphRebuild,
                                          Qt::UniqueConnection);
        m_connectionsChangedConn = connect(graphModel, &GraphModel::connectionsChanged,
                                           this, &GraphViewportItem::requestGraphRebuild,
                                           Qt::UniqueConnection);
    }

    emit graphChanged();
    markSpatialIndexDirty();
    m_graphDirty = true;
    m_nodeDirty = true;
    update();
}

qreal GraphViewportItem::panX() const
{
    return m_panX;
}

void GraphViewportItem::setPanX(qreal value)
{
    if (qFuzzyCompare(m_panX, value))
        return;

    m_panX = value;
    emit panXChanged();
    m_cameraDirty = true;
    update();
}

qreal GraphViewportItem::panY() const
{
    return m_panY;
}

void GraphViewportItem::setPanY(qreal value)
{
    if (qFuzzyCompare(m_panY, value))
        return;

    m_panY = value;
    emit panYChanged();
    m_cameraDirty = true;
    update();
}

qreal GraphViewportItem::zoom() const
{
    return m_zoom;
}

void GraphViewportItem::setZoom(qreal value)
{
    if (qFuzzyCompare(m_zoom, value))
        return;

    // Avoid division-by-zero in viewToWorld.
    if (qFuzzyIsNull(value))
        return;

    m_zoom = value;
    emit zoomChanged();
    m_cameraDirty = true;
    update();
}

bool GraphViewportItem::renderGrid() const
{
    return m_renderGrid;
}

void GraphViewportItem::setRenderGrid(bool value)
{
    if (m_renderGrid == value)
        return;
    m_renderGrid = value;
    emit renderGridChanged();
    m_cameraDirty = true;
    update();
}

bool GraphViewportItem::renderEdges() const
{
    return m_renderEdges;
}

void GraphViewportItem::setRenderEdges(bool value)
{
    if (m_renderEdges == value)
        return;
    m_renderEdges = value;
    emit renderEdgesChanged();
    m_graphDirty = true;
    update();
}

bool GraphViewportItem::renderNodes() const
{
    return m_renderNodes;
}

void GraphViewportItem::setRenderNodes(bool value)
{
    if (m_renderNodes == value)
        return;
    m_renderNodes = value;
    emit renderNodesChanged();
    m_nodeDirty = true;
    m_cameraDirty = true;
    update();
}

qreal GraphViewportItem::baseGridStep() const
{
    return m_baseGridStep;
}

void GraphViewportItem::setBaseGridStep(qreal value)
{
    if (qFuzzyCompare(m_baseGridStep, value))
        return;
    m_baseGridStep = value;
    emit baseGridStepChanged();
    m_cameraDirty = true;
    update();
}

qreal GraphViewportItem::minGridPixelStep() const
{
    return m_minGridPixelStep;
}

void GraphViewportItem::setMinGridPixelStep(qreal value)
{
    if (qFuzzyCompare(m_minGridPixelStep, value))
        return;
    m_minGridPixelStep = value;
    emit minGridPixelStepChanged();
    m_cameraDirty = true;
    update();
}

qreal GraphViewportItem::maxGridPixelStep() const
{
    return m_maxGridPixelStep;
}

void GraphViewportItem::setMaxGridPixelStep(qreal value)
{
    if (qFuzzyCompare(m_maxGridPixelStep, value))
        return;
    m_maxGridPixelStep = value;
    emit maxGridPixelStepChanged();
    m_cameraDirty = true;
    update();
}

QObject *GraphViewportItem::selectedConnection() const
{
    return m_selectedConnection;
}

void GraphViewportItem::setSelectedConnection(QObject *value)
{
    if (m_selectedConnection == value)
        return;
    m_selectedConnection = value;
    emit selectedConnectionChanged();
    m_graphDirty = true;
    update();
}

QObject *GraphViewportItem::selectedComponent() const
{
    return m_selectedComponent;
}

void GraphViewportItem::setSelectedComponent(QObject *value)
{
    if (m_selectedComponent == value)
        return;
    m_selectedComponent = value;
    emit selectedComponentChanged();
    m_nodeDirty = true;
    update();
}

QVariantList GraphViewportItem::selectedComponentIds() const
{
    return m_selectedComponentIds;
}

void GraphViewportItem::setSelectedComponentIds(const QVariantList &value)
{
    if (m_selectedComponentIds == value)
        return;

    m_selectedComponentIds = value;
    m_selectedComponentIdSet.clear();
    for (const QVariant &entry : m_selectedComponentIds) {
        const QString id = entry.toString();
        if (!id.isEmpty())
            m_selectedComponentIdSet.insert(id);
    }

    emit selectedComponentIdsChanged();
    m_nodeDirty = true;
    update();
}

bool GraphViewportItem::tempConnectionDragging() const
{
    return m_tempConnectionDragging;
}

void GraphViewportItem::setTempConnectionDragging(bool value)
{
    if (m_tempConnectionDragging == value)
        return;
    m_tempConnectionDragging = value;
    emit tempConnectionDraggingChanged();
    m_graphDirty = true;
    update();
}

QPointF GraphViewportItem::tempStart() const
{
    return m_tempStart;
}

void GraphViewportItem::setTempStart(const QPointF &value)
{
    if (m_tempStart == value)
        return;
    m_tempStart = value;
    emit tempStartChanged();
    m_graphDirty = true;
    update();
}

QPointF GraphViewportItem::tempEnd() const
{
    return m_tempEnd;
}

void GraphViewportItem::setTempEnd(const QPointF &value)
{
    if (m_tempEnd == value)
        return;
    m_tempEnd = value;
    emit tempEndChanged();
    m_graphDirty = true;
    update();
}

QPointF GraphViewportItem::worldToView(qreal worldX, qreal worldY) const
{
    // viewX = worldX * zoom + panX
    // viewY = worldY * zoom + panY
    return QPointF(worldX * m_zoom + m_panX, worldY * m_zoom + m_panY);
}

QPointF GraphViewportItem::viewToWorld(qreal viewX, qreal viewY) const
{
    // worldX = (viewX - panX) / zoom
    // worldY = (viewY - panY) / zoom
    return QPointF((viewX - m_panX) / m_zoom, (viewY - m_panY) / m_zoom);
}

QVariantMap GraphViewportItem::zoomAtViewAnchor(qreal viewX, qreal viewY,
                                                qreal zoomFactor,
                                                qreal minZoom,
                                                qreal maxZoom,
                                                qreal epsilon) const
{
    QVariantMap result;

    const qreal clampedMin = qMin(minZoom, maxZoom);
    const qreal clampedMax = qMax(minZoom, maxZoom);
    const qreal nextZoom = qBound(clampedMin, m_zoom * zoomFactor, clampedMax);

    if (qAbs(nextZoom - m_zoom) < epsilon) {
        result.insert(QStringLiteral("changed"), false);
        result.insert(QStringLiteral("panX"), m_panX);
        result.insert(QStringLiteral("panY"), m_panY);
        result.insert(QStringLiteral("zoom"), m_zoom);
        return result;
    }

    const QPointF anchorWorld = viewToWorld(viewX, viewY);
    result.insert(QStringLiteral("changed"), true);
    result.insert(QStringLiteral("zoom"), nextZoom);
    result.insert(QStringLiteral("panX"), viewX - anchorWorld.x() * nextZoom);
    result.insert(QStringLiteral("panY"), viewY - anchorWorld.y() * nextZoom);
    return result;
}

QObject *GraphViewportItem::hitTestComponentAtView(qreal viewX, qreal viewY)
{
    ensureSpatialIndex();
    QMutexLocker locker(&m_spatialIndexMutex);
    if (m_indexedComponents.isEmpty())
        return nullptr;

    const QPointF worldPoint = viewToWorld(viewX, viewY);
    const int cx = int(std::floor(worldPoint.x() / m_spatialCellSize));
    const int cy = int(std::floor(worldPoint.y() / m_spatialCellSize));

    const QVector<int> candidates = m_componentCellToIndices.value(cellKey(cx, cy));
    for (int i = candidates.size() - 1; i >= 0; --i) {
        const int idx = candidates.at(i);
        if (idx < 0 || idx >= m_indexedComponents.size())
            continue;
        const IndexedComponent &entry = m_indexedComponents.at(idx);
        if (!entry.component)
            continue;
        if (entry.worldRect.contains(worldPoint))
            return entry.component;
    }

    return nullptr;
}

QObject *GraphViewportItem::hitTestConnectionAtView(qreal viewX, qreal viewY,
                                                    qreal tolerancePx)
{
    ensureSpatialIndex();

    QMutexLocker locker(&m_spatialIndexMutex);
    if (m_indexedConnections.isEmpty())
        return nullptr;

    const QPointF worldPoint = viewToWorld(viewX, viewY);
    const qreal safeZoom = qMax<qreal>(0.001, m_zoom);
    const qreal tolWorld = qMax<qreal>(1.0, tolerancePx / safeZoom);
    const qreal tolSq = tolWorld * tolWorld;

    const QRectF probe(worldPoint.x() - tolWorld,
                       worldPoint.y() - tolWorld,
                       tolWorld * 2.0,
                       tolWorld * 2.0);
    const QRect cells = cellRangeForRect(probe, m_spatialCellSize);

    QSet<int> visited;
    QObject *bestConnection = nullptr;
    qreal bestDistSq = std::numeric_limits<qreal>::max();

    for (int cy = cells.top(); cy <= cells.bottom(); ++cy) {
        for (int cx = cells.left(); cx <= cells.right(); ++cx) {
            const QVector<int> indices = m_connectionCellToIndices.value(cellKey(cx, cy));
            for (int idx : indices) {
                if (idx < 0 || idx >= m_indexedConnections.size() || visited.contains(idx))
                    continue;
                visited.insert(idx);

                const IndexedConnection &entry = m_indexedConnections.at(idx);
                if (!entry.connection)
                    continue;
                if (!entry.worldBounds.adjusted(-tolWorld, -tolWorld,
                                                tolWorld, tolWorld).contains(worldPoint)) {
                    continue;
                }

                for (int segmentIndex = 1; segmentIndex < entry.worldPolyline.size(); ++segmentIndex) {
                    if (segmentIndex - 1 < entry.segmentBounds.size()) {
                        const QRectF segmentProbe = entry.segmentBounds.at(segmentIndex - 1)
                                .adjusted(-tolWorld, -tolWorld, tolWorld, tolWorld);
                        if (!segmentProbe.contains(worldPoint))
                            continue;
                    }

                    const qreal distSq = distanceToSegmentSquared(worldPoint,
                                                                  entry.worldPolyline.at(segmentIndex - 1),
                                                                  entry.worldPolyline.at(segmentIndex));
                    if (distSq <= tolSq && distSq < bestDistSq) {
                        bestDistSq = distSq;
                        bestConnection = entry.connection;
                    }
                }
            }
        }
    }

    return bestConnection;
}

void GraphViewportItem::purgeLabelCache()
{
    // Intentionally no-op in production path until a frame-fenced purge
    // strategy is introduced. Immediate runtime texture teardown during graph
    // resets proved unstable on some environments.
}

int GraphViewportItem::labelTextureCacheSize() const
{
    return m_labelTextureCacheCount.load();
}

qreal GraphViewportItem::rendererMemoryEstimateMb() const
{
    qint64 bytes = 0;

    // Spatial-index payload approximation.
    bytes += qint64(m_indexedComponents.size()) * qint64(sizeof(IndexedComponent));
    bytes += qint64(m_indexedConnections.size()) * qint64(sizeof(IndexedConnection));
    for (auto it = m_componentCellToIndices.constBegin(); it != m_componentCellToIndices.constEnd(); ++it)
        bytes += qint64(it.value().size()) * qint64(sizeof(int));
    for (auto it = m_connectionCellToIndices.constBegin(); it != m_connectionCellToIndices.constEnd(); ++it)
        bytes += qint64(it.value().size()) * qint64(sizeof(int));

    // Label texture bytes are tracked atomically from render thread updates.
    bytes += m_labelTextureCacheBytes.load();

    return qreal(bytes) / (1024.0 * 1024.0);
}

qreal GraphViewportItem::routeRebuildLastMs() const
{
    return m_routeRebuildLastMs.load();
}

qreal GraphViewportItem::routeRebuildP50Ms() const
{
    return m_routeRebuildP50Ms.load();
}

qreal GraphViewportItem::routeRebuildP95Ms() const
{
    return m_routeRebuildP95Ms.load();
}

int GraphViewportItem::routeRebuildSampleCount() const
{
    return m_routeRebuildSampleCount.load();
}

void GraphViewportItem::repaint()
{
    m_cameraDirty = true;
    update();
}

void GraphViewportItem::requestGraphRebuild()
{
    markSpatialIndexDirty();
    m_graphDirty = true;
    m_nodeDirty = true;
    scheduleGraphRebuild();
    update();
}

void GraphViewportItem::scheduleGraphRebuild()
{
    if (m_graphRebuildScheduled)
        return;

    m_graphRebuildScheduled = true;
    QTimer::singleShot(0, this, [this]() {
        executeScheduledGraphRebuild();
    });
}

void GraphViewportItem::executeScheduledGraphRebuild()
{
    m_graphRebuildScheduled = false;
    ensureSpatialIndex();
    m_graphDirty = true;
    m_nodeDirty = true;
    update();
}

void GraphViewportItem::updateLodState()
{
    const bool simpleEdges = m_zoom < 0.60;
    const bool hideLabels = m_zoom < 0.68;
    const bool hideOutlines = m_zoom < 0.50;

    const bool edgeModeChanged = (simpleEdges != m_lodSimpleEdges);
    if (edgeModeChanged)
        m_lodSimpleEdges = simpleEdges;

    const bool labelModeChanged = (hideLabels != m_lodHideNodeLabels);
    if (labelModeChanged)
        m_lodHideNodeLabels = hideLabels;

    const bool outlineModeChanged = (hideOutlines != m_lodHideNodeOutlines);
    if (outlineModeChanged)
        m_lodHideNodeOutlines = hideOutlines;

    if (edgeModeChanged) {
        auto *normalGeom = m_normalEdgesGeomNode ? m_normalEdgesGeomNode->geometry() : nullptr;
        auto *selectedGeom = m_selectedEdgesGeomNode ? m_selectedEdgesGeomNode->geometry() : nullptr;
        if (normalGeom) {
            normalGeom->setLineWidth(simpleEdges ? 1.0f : 2.0f);
            m_normalEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
        }
        if (selectedGeom) {
            selectedGeom->setLineWidth(simpleEdges ? 1.0f : 3.0f);
            m_selectedEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
        }

        auto *normalMat = m_normalEdgesGeomNode
            ? static_cast<QSGFlatColorMaterial *>(m_normalEdgesGeomNode->material())
            : nullptr;
        auto *selectedMat = m_selectedEdgesGeomNode
            ? static_cast<QSGFlatColorMaterial *>(m_selectedEdgesGeomNode->material())
            : nullptr;
        const QColor normalColor = simpleEdges
            ? QColor(QStringLiteral("#546e7a"))
            : QColor(QStringLiteral("#607d8b"));
        const QColor selectedColor = simpleEdges
            ? QColor(QStringLiteral("#546e7a"))
            : QColor(QStringLiteral("#ff5722"));
        if (normalMat) {
            normalMat->setColor(normalColor);
            m_normalEdgesGeomNode->markDirty(QSGNode::DirtyMaterial);
        }
        if (selectedMat) {
            selectedMat->setColor(selectedColor);
            m_selectedEdgesGeomNode->markDirty(QSGNode::DirtyMaterial);
        }
    }

    if (labelModeChanged || outlineModeChanged)
        m_nodeDirty = true;
}

void GraphViewportItem::requestNodeRepaint()
{
    m_nodeDirty = true;
    update();
}

QSGNode *GraphViewportItem::updatePaintNode(QSGNode *oldNode,
                                            UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData)

    // -----------------------------------------------------------------------
    // First-time setup: build the persistent node tree.
    //
    // Node layout:
    //   m_rootNode
    //   ├── m_gridGeomNode          (screen-space grid lines)
    //   ├── m_edgesTransformNode    (camera world→screen transform)
    //   │   ├── m_normalEdgesGeomNode   (world-space, normal edges)
    //   │   ├── m_selectedEdgesGeomNode (world-space, selected edge)
    //   │   ├── m_normalArrowsGeomNode  (world-space, normal arrowheads)
    //   │   └── m_selectedArrowsGeomNode(world-space, selected arrowheads)
    //   └── m_tempEdgeGeomNode      (screen-space, drag-in-progress edge)
    // -----------------------------------------------------------------------
    if (!oldNode) {
        // Scene graph was recreated; cached node pointers from a previous graph
        // lifetime are no longer valid.
        m_labelNodes.clear();

        m_labelTextureCache.clear();
        m_labelTextureCacheCount.store(0);
        m_labelTextureCacheBytes.store(0);
        m_labelCachePurgeRequested = false;

        m_rootNode = new QSGNode();

        m_gridGeomNode = createEmptyLineNode(QColor(QStringLiteral("#e0e0e0")), 1.0f);
        m_rootNode->appendChildNode(m_gridGeomNode);

        m_nodesRootNode = new QSGNode();
        m_nodesTransformNode = new QSGTransformNode();
        m_nodeFillGeomNode = createEmptyColoredNode();
        m_nodeOutlineGeomNode = createEmptyColoredNode();
        m_nodeLabelsRootNode = new QSGNode();
        m_nodesTransformNode->appendChildNode(m_nodeFillGeomNode);
        m_nodesTransformNode->appendChildNode(m_nodeOutlineGeomNode);
        m_nodesTransformNode->appendChildNode(m_nodeLabelsRootNode);
        m_nodesRootNode->appendChildNode(m_nodesTransformNode);
        m_rootNode->appendChildNode(m_nodesRootNode);

        m_edgesTransformNode    = new QSGTransformNode();
        m_normalEdgesGeomNode   = createEmptyLineNode(QColor(QStringLiteral("#607d8b")), 2.0f);
        m_selectedEdgesGeomNode = createEmptyLineNode(QColor(QStringLiteral("#ff5722")), 3.0f);
        m_normalArrowsGeomNode = createEmptyColoredNode();
        m_selectedArrowsGeomNode = createEmptyColoredNode();
        m_edgesTransformNode->appendChildNode(m_normalEdgesGeomNode);
        m_edgesTransformNode->appendChildNode(m_selectedEdgesGeomNode);
        m_edgesTransformNode->appendChildNode(m_normalArrowsGeomNode);
        m_edgesTransformNode->appendChildNode(m_selectedArrowsGeomNode);
        m_rootNode->appendChildNode(m_edgesTransformNode);

        m_tempEdgeGeomNode = createEmptyLineNode(QColor(QStringLiteral("#90caf9")), 2.0f);
        m_rootNode->appendChildNode(m_tempEdgeGeomNode);

        // Force a full update on the first frame.
        m_graphDirty  = true;
        m_cameraDirty = true;
        m_nodeDirty = true;
        oldNode = m_rootNode;
    }

    if (width() <= 0 || height() <= 0)
        return oldNode;

    if (m_labelCachePurgeRequested)
        clearLabelTexturesOnRenderThread();

    // -----------------------------------------------------------------------
    // Graph dirty: rebuild edge/temp geometry in world space (O(N+E)).
    // Happens only when topology or selection changes — NOT on every pan frame.
    // -----------------------------------------------------------------------
    if (m_graphDirty) {
        QElapsedTimer routeRebuildTimer;
        routeRebuildTimer.start();
        updateEdgesGeometry();
        recordRouteRebuildSample(qreal(routeRebuildTimer.nsecsElapsed()) / 1000000.0);
        updateTempEdgeGeometry();
        m_graphDirty = false;
    }

    // -----------------------------------------------------------------------
    // Camera dirty: update grid (screen-space) + edge camera matrix.
    // One 4×4 matrix write replaces O(E) vertex position recalculation.
    // -----------------------------------------------------------------------
    if (m_cameraDirty) {
        updateGridGeometry();
        updateLodState();

        QMatrix4x4 cam;
        cam.setToIdentity();
        cam.translate(float(m_panX), float(m_panY));
        cam.scale(float(m_zoom));
        m_edgesTransformNode->setMatrix(cam);
        if (m_nodesTransformNode)
            m_nodesTransformNode->setMatrix(cam);

        m_cameraDirty = false;
    }

    if (m_nodeDirty) {
        updateNodeGeometry();
        m_nodeDirty = false;
    }

    return oldNode;
}

qreal GraphViewportItem::normalizedGridStep() const
{
    qreal step = m_baseGridStep * m_zoom;
    if (step <= 0.0)
        return 0.0;

    while (step < m_minGridPixelStep)
        step *= 2.0;
    while (step > m_maxGridPixelStep)
        step /= 2.0;
    return step;
}

qreal GraphViewportItem::positiveModulo(qreal value, qreal modulus)
{
    if (modulus == 0.0)
        return 0.0;
    return std::fmod(std::fmod(value, modulus) + modulus, modulus);
}

// ---------------------------------------------------------------------------
// updateGridGeometry
// Rebuilds the grid vertex buffer in screen space.  Called from the render
// thread when m_cameraDirty is true.  Re-uses the existing QSGGeometry
// buffer, only calling allocate() when the line count changes.
// ---------------------------------------------------------------------------
void GraphViewportItem::updateGridGeometry()
{
    auto *geom = m_gridGeomNode->geometry();

    if (!m_renderGrid || width() <= 0 || height() <= 0) {
        if (geom->vertexCount() > 0) {
            geom->allocate(0);
            m_gridGeomNode->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    const qreal step = normalizedGridStep();
    if (step <= 0.0) {
        if (geom->vertexCount() > 0) {
            geom->allocate(0);
            m_gridGeomNode->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    const qreal offsetX = positiveModulo(m_panX, step);
    const qreal offsetY = positiveModulo(m_panY, step);

    // Pre-count to allocate exact buffer size (avoids mid-fill resize).
    int verts = 0;
    for (qreal gx = -step + offsetX; gx < width() + step; gx += step)
        verts += 2;
    for (qreal gy = -step + offsetY; gy < height() + step; gy += step)
        verts += 2;

    if (geom->vertexCount() != verts)
        geom->allocate(verts);

    auto *v = geom->vertexDataAsPoint2D();
    int idx = 0;
    for (qreal gx = -step + offsetX; gx < width() + step; gx += step) {
        v[idx++].set(float(gx), 0.0f);
        v[idx++].set(float(gx), float(height()));
    }
    for (qreal gy = -step + offsetY; gy < height() + step; gy += step) {
        v[idx++].set(0.0f, float(gy));
        v[idx++].set(float(width()), float(gy));
    }

    m_gridGeomNode->markDirty(QSGNode::DirtyGeometry);
}

// ---------------------------------------------------------------------------
// updateEdgesGeometry
// Rebuilds edge vertex buffers in WORLD space.  Called from the render
// thread only when m_graphDirty is true (topology / selection change).
//
// Vertices are world-space coordinates; the parent QSGTransformNode applies
// the camera matrix so that pan/zoom only requires a matrix update, not a
// full vertex rebuild.
// ---------------------------------------------------------------------------
void GraphViewportItem::updateEdgesGeometry()
{
    auto clearNode = [](QSGGeometryNode *n) {
        if (n->geometry()->vertexCount() > 0) {
            n->geometry()->allocate(0);
            n->markDirty(QSGNode::DirtyGeometry);
        }
    };

    auto appendArrowTriangle = [](QSGGeometry::ColoredPoint2D *verts,
                                  int &idx,
                                  const QPointF &tip,
                                  const QPointF &from,
                                  const QColor &color,
                                  qreal length,
                                  qreal width) {
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
    };

    if (!m_renderEdges) {
        clearNode(m_normalEdgesGeomNode);
        clearNode(m_selectedEdgesGeomNode);
        clearNode(m_normalArrowsGeomNode);
        clearNode(m_selectedArrowsGeomNode);
        return;
    }

    auto *graphModel = qobject_cast<GraphModel *>(m_graph);
    if (!graphModel) {
        clearNode(m_normalEdgesGeomNode);
        clearNode(m_selectedEdgesGeomNode);
        clearNode(m_normalArrowsGeomNode);
        clearNode(m_selectedArrowsGeomNode);
        return;
    }

    const auto &components = graphModel->componentList();
    const auto &connections = graphModel->connectionList();

    // O(N): build id→pointer map to avoid O(N) componentById() per edge.
    QHash<QString, ComponentModel *> componentById;
    componentById.reserve(components.size());
    for (ComponentModel *c : components)
        componentById.insert(c->id(), c);

    // Pre-count vertices (2 per routed segment) and arrow triangles.
    int normalCount = 0, selectedCount = 0;
    int normalArrowCount = 0, selectedArrowCount = 0;
    for (ConnectionModel *conn : connections) {
        ComponentModel *src = componentById.value(conn->sourceId(), nullptr);
        ComponentModel *tgt = componentById.value(conn->targetId(), nullptr);
        if (!src || !tgt)
            continue;

        const QVector<QPointF> route = orthogonalRouteForConnection(conn, src, tgt, components);
        const int segmentCount = qMax(0, route.size() - 1);
        if (static_cast<QObject *>(conn) == m_selectedConnection)
            selectedCount += segmentCount;
        else
            normalCount += segmentCount;

        if (route.size() >= 2) {
            if (static_cast<QObject *>(conn) == m_selectedConnection)
                ++selectedArrowCount;
            else
                ++normalArrowCount;
        }
    }

    // Reallocate only if the edge count changed.
    if (m_normalEdgesGeomNode->geometry()->vertexCount() != normalCount * 2)
        m_normalEdgesGeomNode->geometry()->allocate(normalCount * 2);
    if (m_selectedEdgesGeomNode->geometry()->vertexCount() != selectedCount * 2)
        m_selectedEdgesGeomNode->geometry()->allocate(selectedCount * 2);
    if (m_normalArrowsGeomNode->geometry()->vertexCount() != normalArrowCount * 3)
        m_normalArrowsGeomNode->geometry()->allocate(normalArrowCount * 3);
    if (m_selectedArrowsGeomNode->geometry()->vertexCount() != selectedArrowCount * 3)
        m_selectedArrowsGeomNode->geometry()->allocate(selectedArrowCount * 3);

    auto *normalV   = m_normalEdgesGeomNode->geometry()->vertexDataAsPoint2D();
    auto *selectedV = m_selectedEdgesGeomNode->geometry()->vertexDataAsPoint2D();
    auto *normalArrowV = m_normalArrowsGeomNode->geometry()->vertexDataAsColoredPoint2D();
    auto *selectedArrowV = m_selectedArrowsGeomNode->geometry()->vertexDataAsColoredPoint2D();
    int nIdx = 0, sIdx = 0;
    int nArrowIdx = 0, sArrowIdx = 0;

    const qreal arrowLength = m_lodSimpleEdges ? 8.0 : 12.0;
    const qreal arrowWidth = m_lodSimpleEdges ? 3.5 : 5.0;
    const QColor normalColor = m_lodSimpleEdges
        ? QColor(QStringLiteral("#546e7a"))
        : QColor(QStringLiteral("#607d8b"));
    const QColor selectedColor = m_lodSimpleEdges
        ? QColor(QStringLiteral("#546e7a"))
        : QColor(QStringLiteral("#ff5722"));

    for (ConnectionModel *conn : connections) {
        ComponentModel *src = componentById.value(conn->sourceId(), nullptr);
        ComponentModel *tgt = componentById.value(conn->targetId(), nullptr);
        if (!src || !tgt)
            continue;

        const QVector<QPointF> route = orthogonalRouteForConnection(conn, src, tgt, components);
        if (route.size() < 2)
            continue;

        // World-space coordinates — QSGTransformNode handles world→screen.
        if (static_cast<QObject *>(conn) == m_selectedConnection) {
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

    m_normalEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
    m_selectedEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
    m_normalArrowsGeomNode->markDirty(QSGNode::DirtyGeometry);
    m_selectedArrowsGeomNode->markDirty(QSGNode::DirtyGeometry);
}

// ---------------------------------------------------------------------------
// updateTempEdgeGeometry
// Updates the in-progress connection drag line.  Coordinates are in screen
// (view) space and sit outside the camera QSGTransformNode.
// ---------------------------------------------------------------------------
void GraphViewportItem::updateTempEdgeGeometry()
{
    auto *geom = m_tempEdgeGeomNode->geometry();

    if (!m_tempConnectionDragging) {
        if (geom->vertexCount() > 0) {
            geom->allocate(0);
            m_tempEdgeGeomNode->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    if (geom->vertexCount() != 2)
        geom->allocate(2);

    auto *v = geom->vertexDataAsPoint2D();
    v[0].set(float(m_tempStart.x()), float(m_tempStart.y()));
    v[1].set(float(m_tempEnd.x()),   float(m_tempEnd.y()));
    m_tempEdgeGeomNode->markDirty(QSGNode::DirtyGeometry);
}

QVector<int> GraphViewportItem::visibleComponentIndices() const
{
    QVector<int> result;
    if (width() <= 0 || height() <= 0)
        return result;

    QMutexLocker locker(&m_spatialIndexMutex);
    if (m_indexedComponents.isEmpty())
        return result;

    const QPointF worldTopLeft = viewToWorld(0.0, 0.0);
    const QPointF worldBottomRight = viewToWorld(width(), height());
    const qreal marginWorld = qMax<qreal>(40.0, 120.0 / qMax<qreal>(0.001, m_zoom));
    const QRectF visibleWorld(qMin(worldTopLeft.x(), worldBottomRight.x()) - marginWorld,
                              qMin(worldTopLeft.y(), worldBottomRight.y()) - marginWorld,
                              qAbs(worldBottomRight.x() - worldTopLeft.x()) + marginWorld * 2.0,
                              qAbs(worldBottomRight.y() - worldTopLeft.y()) + marginWorld * 2.0);

    const QRect cells = cellRangeForRect(visibleWorld, m_spatialCellSize);
    QSet<int> seen;
    for (int cy = cells.top(); cy <= cells.bottom(); ++cy) {
        for (int cx = cells.left(); cx <= cells.right(); ++cx) {
            const QVector<int> indices = m_componentCellToIndices.value(cellKey(cx, cy));
            for (int idx : indices) {
                if (idx < 0 || idx >= m_indexedComponents.size() || seen.contains(idx))
                    continue;
                const IndexedComponent &entry = m_indexedComponents.at(idx);
                if (!entry.component || !entry.worldRect.intersects(visibleWorld))
                    continue;
                seen.insert(idx);
                result.append(idx);
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

QVector<GraphViewportItem::IndexedComponent> GraphViewportItem::visibleComponentsSnapshot() const
{
    QVector<IndexedComponent> result;
    const QVector<int> visible = visibleComponentIndices();
    if (visible.isEmpty())
        return result;

    QMutexLocker locker(&m_spatialIndexMutex);
    result.reserve(visible.size());
    for (int idx : visible) {
        if (idx < 0 || idx >= m_indexedComponents.size())
            continue;
        result.append(m_indexedComponents.at(idx));
    }
    return result;
}

void GraphViewportItem::updateNodeGeometry()
{
    auto clearColoredNode = [](QSGGeometryNode *node) {
        if (node && node->geometry()->vertexCount() > 0) {
            node->geometry()->allocate(0);
            node->markDirty(QSGNode::DirtyGeometry);
        }
    };

    if (!m_renderNodes || !m_nodesRootNode) {
        clearColoredNode(m_nodeFillGeomNode);
        clearColoredNode(m_nodeOutlineGeomNode);
        updateLabelNodes();
        return;
    }

    const QVector<IndexedComponent> visible = visibleComponentsSnapshot();

    const int fillVertexCount = visible.size() * 6;
    if (m_nodeFillGeomNode->geometry()->vertexCount() != fillVertexCount)
        m_nodeFillGeomNode->geometry()->allocate(fillVertexCount);

    const int outlineVertexCount = m_lodHideNodeOutlines ? 0 : visible.size() * 24;
    if (m_nodeOutlineGeomNode->geometry()->vertexCount() != outlineVertexCount)
        m_nodeOutlineGeomNode->geometry()->allocate(outlineVertexCount);

    auto *fillVerts = m_nodeFillGeomNode->geometry()->vertexDataAsColoredPoint2D();
    auto *outlineVerts = m_nodeOutlineGeomNode->geometry()->vertexDataAsColoredPoint2D();
    int fillIdx = 0;
    int outlineIdx = 0;

    auto appendTriangleRect = [](QSGGeometry::ColoredPoint2D *verts, int &idx,
                                 const QRectF &rect, const QColor &color) {
        const unsigned char r = color.red();
        const unsigned char g = color.green();
        const unsigned char b = color.blue();
        const unsigned char a = color.alpha();
        const float left = float(rect.left());
        const float right = float(rect.right());
        const float top = float(rect.top());
        const float bottom = float(rect.bottom());

        verts[idx++].set(left, top, r, g, b, a);
        verts[idx++].set(right, top, r, g, b, a);
        verts[idx++].set(left, bottom, r, g, b, a);
        verts[idx++].set(right, top, r, g, b, a);
        verts[idx++].set(right, bottom, r, g, b, a);
        verts[idx++].set(left, bottom, r, g, b, a);
    };

    for (const IndexedComponent &entry : visible) {
        ComponentModel *component = entry.component;
        if (!component)
            continue;

        const QRectF viewRect = entry.worldRect;

        const QColor fillColor(component->color());
        appendTriangleRect(fillVerts, fillIdx, viewRect, fillColor);

         if (!m_lodHideNodeOutlines) {
             const bool selected = (static_cast<QObject *>(component) == m_selectedComponent)
                                   || m_selectedComponentIdSet.contains(component->id());
             const qreal borderWidth = selected ? 2.5 : 1.5;
             const QColor borderColor = selected
              ? QColor(QStringLiteral("#ff5722"))
              : fillColor.darker(140);

             appendTriangleRect(outlineVerts, outlineIdx,
                       QRectF(viewRect.left(), viewRect.top(), viewRect.width(), borderWidth),
                       borderColor);
             appendTriangleRect(outlineVerts, outlineIdx,
                       QRectF(viewRect.left(), viewRect.bottom() - borderWidth, viewRect.width(), borderWidth),
                       borderColor);
             appendTriangleRect(outlineVerts, outlineIdx,
                       QRectF(viewRect.left(), viewRect.top() + borderWidth, borderWidth,
                           qMax<qreal>(0.0, viewRect.height() - borderWidth * 2.0)),
                       borderColor);
             appendTriangleRect(outlineVerts, outlineIdx,
                       QRectF(viewRect.right() - borderWidth, viewRect.top() + borderWidth, borderWidth,
                           qMax<qreal>(0.0, viewRect.height() - borderWidth * 2.0)),
                       borderColor);
         }
    }

    m_nodeFillGeomNode->markDirty(QSGNode::DirtyGeometry);
    m_nodeOutlineGeomNode->markDirty(QSGNode::DirtyGeometry);
    updateLabelNodes();
}

QString GraphViewportItem::labelCacheKey(const ComponentModel *component) const
{
    if (!component)
        return QString();
    return component->title() + QStringLiteral("|")
        + QString::number(qHash(component->content())) + QStringLiteral("|")
        + component->icon() + QStringLiteral("|")
        + QString::number(int(std::round(component->width())))
        + QStringLiteral("|")
        + QString::number(int(std::round(component->height())))
        + QStringLiteral("|") + QString::number(window() ? window()->effectiveDevicePixelRatio() : 1.0, 'f', 2);
}

void GraphViewportItem::updateLabelNodes()
{
    if (!m_nodeLabelsRootNode)
        return;

    const QVector<IndexedComponent> visible = (m_renderNodes ? visibleComponentsSnapshot()
                                                            : QVector<IndexedComponent>());

    QVector<LabelTextureNode *> labelNodes;
    for (QSGNode *child = m_nodeLabelsRootNode->firstChild();
         child != nullptr;
         child = child->nextSibling()) {
        labelNodes.append(static_cast<LabelTextureNode *>(child));
    }

    while (labelNodes.size() < visible.size()) {
        auto *node = new LabelTextureNode();
        node->setOwnsTexture(true);
        m_nodeLabelsRootNode->appendChildNode(node);
        labelNodes.append(node);
    }

    // Avoid runtime texture destruction during active rendering paths.

    const qreal devicePixelRatio = window() ? window()->effectiveDevicePixelRatio() : 1.0;

    QFont titleFont;
    titleFont.setBold(true);
    titleFont.setPixelSize(11);
    QFontMetrics titleMetrics(titleFont);

    QFont contentFont;
    contentFont.setPixelSize(10);

    QFont iconFont;
    iconFont.setPixelSize(11);

    // Use cached FontAwesome icon font configuration only if it has been
    // initialized on the GUI thread. Do not call FontAwesome helpers from
    // the scene graph render thread, as they may trigger lazy loading with
    // file I/O and QFontDatabase operations.
    if (g_iconFontInitialized) {
        iconFont.setFamily(g_iconFontFamilySolid);
        iconFont.setWeight(static_cast<QFont::Weight>(g_iconFontWeightSolid));
    }

    int activeLabelTextures = 0;

    for (int i = 0; i < labelNodes.size(); ++i) {
        LabelTextureNode *node = labelNodes.at(i);
        if (i >= visible.size() || !m_renderNodes || m_lodHideNodeLabels) {
            node->setRect(0, 0, 0, 0);
            continue;
        }

        const IndexedComponent &entry = visible.at(i);
        ComponentModel *component = entry.component;
        if (!component) {
            node->setRect(0, 0, 0, 0);
            continue;
        }

        const int availableWidth = qMax(16, int(std::round(component->width() - 12.0)));
        const int availableHeight = qMax(24, int(std::round(component->height() - 12.0)));
        if (availableWidth <= 0 || availableHeight <= 0 || !window()) {
            node->setRect(0, 0, 0, 0);
            continue;
        }

        const QString key = labelCacheKey(component)
                + QStringLiteral("|") + QString::number(availableWidth)
                + QStringLiteral("|") + QString::number(availableHeight);
        if (node->cacheKey != key || !node->texture()) {
            QImage image(QSize(int(std::ceil(availableWidth * devicePixelRatio)),
                               int(std::ceil(availableHeight * devicePixelRatio))),
                         QImage::Format_ARGB32_Premultiplied);
            image.setDevicePixelRatio(devicePixelRatio);
            image.fill(Qt::transparent);

            QPainter painter(&image);
            painter.setRenderHint(QPainter::TextAntialiasing, true);

            const int headerHeight = qBound(20, int(std::round(availableHeight * 0.30)), 28);
            painter.fillRect(QRectF(0.0, 0.0, availableWidth, headerHeight), QColor(0, 0, 0, 48));

            const QString iconName = component->icon().isEmpty() ? QStringLiteral("cube") : component->icon();
            const QString iconGlyph = FontAwesome::iconForCpp(iconName, FontAwesome::Solid);
            int textLeft = 6;
            if (!iconGlyph.isEmpty()) {
                painter.setFont(iconFont);
                painter.setPen(Qt::white);
                const QRectF iconRect(6.0, 0.0, 14.0, headerHeight);
                painter.drawText(iconRect, Qt::AlignCenter, iconGlyph);
                textLeft = 24;
            }

            painter.setFont(titleFont);
            painter.setPen(Qt::white);
            const int titleAvail = qMax(8, availableWidth - textLeft - 6);
            const QString title = titleMetrics.elidedText(component->title(), Qt::ElideRight, titleAvail);
            painter.drawText(QRectF(textLeft, 0.0, titleAvail, headerHeight),
                             Qt::AlignVCenter | Qt::AlignLeft,
                             title);

            const QString content = component->content().trimmed();
            if (!content.isEmpty()) {
                painter.setFont(contentFont);
                painter.setPen(QColor(QStringLiteral("#e8f1f6")));
                const QRectF contentRect(4.0,
                                         headerHeight + 2.0,
                                         availableWidth - 8.0,
                                         qMax(8.0, qreal(availableHeight - headerHeight - 4.0)));
                painter.drawText(contentRect,
                                 Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap,
                                 content);
            }

            painter.end();

            QSGTexture *texture = window()->createTextureFromImage(image);
            if (texture) {
                if (node->textureBytes > 0)
                    m_labelTextureCacheBytes.fetch_sub(node->textureBytes);
                node->textureBytes = qint64(image.sizeInBytes());
                m_labelTextureCacheBytes.fetch_add(node->textureBytes);
                node->cacheKey = key;
                node->setTexture(texture);
            }
        }

        if (node->texture())
            ++activeLabelTextures;

        const qreal viewWidth = entry.worldRect.width();
        const qreal viewHeight = entry.worldRect.height();
        node->setRect(QRectF(entry.worldRect.left() + 6.0,
                     entry.worldRect.top() + 6.0,
                     qMax<qreal>(8.0, viewWidth - 12.0),
                     qMax<qreal>(8.0, viewHeight - 12.0)));
    }

    m_labelTextureCacheCount.store(activeLabelTextures);
}

void GraphViewportItem::clearLabelTexturesOnRenderThread()
{
    // Walk the current scene-graph subtree instead of relying on cached
    // pointers which can be stale after scene-graph invalidation.
    if (m_nodeLabelsRootNode) {
        for (QSGNode *child = m_nodeLabelsRootNode->firstChild();
             child != nullptr;
             child = child->nextSibling()) {
            auto *node = static_cast<LabelTextureNode *>(child);
            node->cacheKey.clear();
            node->textureBytes = 0;
            node->setTexture(nullptr);
        }
    }

    m_labelTextureCache.clear();
    m_labelNodes.clear();
    m_labelTextureCacheCount.store(0);
    m_labelTextureCacheBytes.store(0);
    m_labelCachePurgeRequested = false;
}

void GraphViewportItem::markSpatialIndexDirty()
{
    m_spatialIndexDirty = true;
}

void GraphViewportItem::ensureSpatialIndex()
{
    if (!m_spatialIndexDirty)
        return;

    if (QThread::currentThread() != thread())
        return;

    rebuildSpatialIndex();
}

void GraphViewportItem::clearComponentGeometryConnections()
{
    for (const QMetaObject::Connection &conn : m_componentGeometryChangedConns)
        QObject::disconnect(conn);
    m_componentGeometryChangedConns.clear();
}

void GraphViewportItem::clearConnectionGeometryConnections()
{
    for (const QMetaObject::Connection &conn : m_connectionGeometryChangedConns)
        QObject::disconnect(conn);
    m_connectionGeometryChangedConns.clear();
}

void GraphViewportItem::rebuildSpatialIndex()
{
    QVector<IndexedComponent> indexedComponents;
    QVector<IndexedConnection> indexedConnections;
    QHash<quint64, QVector<int>> componentCellToIndices;
    QHash<quint64, QVector<int>> connectionCellToIndices;
    clearComponentGeometryConnections();
    clearConnectionGeometryConnections();

    auto *graphModel = qobject_cast<GraphModel *>(m_graph);
    if (!graphModel) {
        QMutexLocker locker(&m_spatialIndexMutex);
        m_indexedComponents.clear();
        m_indexedConnections.clear();
        m_componentCellToIndices.clear();
        m_connectionCellToIndices.clear();
        m_spatialIndexDirty = false;
        return;
    }

    const auto &components = graphModel->componentList();
    indexedComponents.reserve(components.size());

    for (ComponentModel *component : components) {
        if (!component)
            continue;

        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::xChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::yChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::widthChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::heightChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::titleChanged,
                this, &GraphViewportItem::requestNodeRepaint));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::contentChanged,
                this, &GraphViewportItem::requestNodeRepaint));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::iconChanged,
                this, &GraphViewportItem::requestNodeRepaint));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::colorChanged,
                this, &GraphViewportItem::requestNodeRepaint));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::shapeChanged,
                this, &GraphViewportItem::requestNodeRepaint));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::idChanged,
                this, &GraphViewportItem::requestGraphRebuild));

        const QRectF rect(component->x() - component->width() / 2.0,
                          component->y() - component->height() / 2.0,
                          component->width(),
                          component->height());

        const int idx = indexedComponents.size();
        indexedComponents.push_back(IndexedComponent { component, rect });

        const QRect cells = cellRangeForRect(rect, m_spatialCellSize);
        for (int cy = cells.top(); cy <= cells.bottom(); ++cy) {
            for (int cx = cells.left(); cx <= cells.right(); ++cx)
                componentCellToIndices[cellKey(cx, cy)].append(idx);
        }
    }

    const auto &connections = graphModel->connectionList();
    indexedConnections.reserve(connections.size());

    QHash<QString, ComponentModel *> componentById;
    componentById.reserve(components.size());
    for (ComponentModel *component : components) {
        if (component)
            componentById.insert(component->id(), component);
    }

    for (ConnectionModel *connection : connections) {
        if (!connection)
            continue;

        m_connectionGeometryChangedConns.append(
            connect(connection, &ConnectionModel::sourceIdChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_connectionGeometryChangedConns.append(
            connect(connection, &ConnectionModel::targetIdChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_connectionGeometryChangedConns.append(
            connect(connection, &ConnectionModel::idChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_connectionGeometryChangedConns.append(
            connect(connection, &ConnectionModel::sourceSideChanged,
                    this, &GraphViewportItem::requestGraphRebuild));
        m_connectionGeometryChangedConns.append(
            connect(connection, &ConnectionModel::targetSideChanged,
                    this, &GraphViewportItem::requestGraphRebuild));

        ComponentModel *src = componentById.value(connection->sourceId(), nullptr);
        ComponentModel *tgt = componentById.value(connection->targetId(), nullptr);
        if (!src || !tgt)
            continue;

        const QVector<QPointF> route = orthogonalRouteForConnection(connection, src, tgt, components);
        if (route.size() < 2)
            continue;

        const QRectF bounds = polylineBounds(route);
        QVector<QRectF> segmentBounds;
        segmentBounds.reserve(qMax(0, route.size() - 1));
        for (int i = 1; i < route.size(); ++i) {
            const QPointF &a = route.at(i - 1);
            const QPointF &b = route.at(i);
            segmentBounds.append(QRectF(QPointF(qMin(a.x(), b.x()), qMin(a.y(), b.y())),
                                        QPointF(qMax(a.x(), b.x()), qMax(a.y(), b.y()))));
        }

        const int idx = indexedConnections.size();
        indexedConnections.push_back(IndexedConnection {
            connection,
            route,
            segmentBounds,
            bounds
        });

        const QRect cells = cellRangeForRect(bounds, m_spatialCellSize);
        for (int cy = cells.top(); cy <= cells.bottom(); ++cy) {
            for (int cx = cells.left(); cx <= cells.right(); ++cx)
                connectionCellToIndices[cellKey(cx, cy)].append(idx);
        }
    }

    {
        QMutexLocker locker(&m_spatialIndexMutex);
        m_indexedComponents.swap(indexedComponents);
        m_indexedConnections.swap(indexedConnections);
        m_componentCellToIndices.swap(componentCellToIndices);
        m_connectionCellToIndices.swap(connectionCellToIndices);
    }

    m_spatialIndexDirty = false;
}

quint64 GraphViewportItem::cellKey(int cx, int cy)
{
    return (quint64(quint32(cx)) << 32) | quint32(cy);
}

QRect GraphViewportItem::cellRangeForRect(const QRectF &rect, qreal cellSize)
{
    if (cellSize <= 0.0)
        return QRect(0, 0, 1, 1);

    const int minX = int(std::floor(rect.left() / cellSize));
    const int maxX = int(std::floor(rect.right() / cellSize));
    const int minY = int(std::floor(rect.top() / cellSize));
    const int maxY = int(std::floor(rect.bottom() / cellSize));
    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

qreal GraphViewportItem::distanceToSegmentSquared(const QPointF &point,
                                                  const QPointF &a,
                                                  const QPointF &b)
{
    const QPointF ab = b - a;
    const qreal abLenSq = ab.x() * ab.x() + ab.y() * ab.y();
    if (qFuzzyIsNull(abLenSq)) {
        const QPointF ap = point - a;
        return ap.x() * ap.x() + ap.y() * ap.y();
    }

    const QPointF ap = point - a;
    const qreal t = qBound<qreal>(0.0,
                                  (ap.x() * ab.x() + ap.y() * ab.y()) / abLenSq,
                                  1.0);
    const QPointF closest(a.x() + ab.x() * t, a.y() + ab.y() * t);
    const QPointF d = point - closest;
    return d.x() * d.x() + d.y() * d.y();
}

qreal GraphViewportItem::percentile(const QVector<qreal> &samples, qreal p)
{
    if (samples.isEmpty())
        return 0.0;

    QVector<qreal> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const int idx = qBound(0, int(p * (sorted.size() - 1)), sorted.size() - 1);
    return sorted.at(idx);
}

void GraphViewportItem::recordRouteRebuildSample(qreal ms)
{
    if (ms <= 0.0 || ms > 2000.0)
        return;

    m_routeRebuildSamples.push_back(ms);
    if (m_routeRebuildSamples.size() > m_routeRebuildMaxSamples)
        m_routeRebuildSamples.removeFirst();

    m_routeRebuildLastMs.store(ms);
    m_routeRebuildSampleCount.store(m_routeRebuildSamples.size());

    // Recompute quantiles at low overhead cadence.
    if (m_routeRebuildSamples.size() < 2)
        return;

    if ((m_routeRebuildSamples.size() % 8) == 0) {
        m_routeRebuildP50Ms.store(percentile(m_routeRebuildSamples, 0.50));
        m_routeRebuildP95Ms.store(percentile(m_routeRebuildSamples, 0.95));
    }
}

