#include "GraphViewportItem.h"

#include "ComponentModel.h"
#include "ConnectionModel.h"
#include "GraphModel.h"

#include <QColor>
#include <QHash>
#include <QMatrix4x4>
#include <QRect>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGTransformNode>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <limits>
#include <QtGlobal>
#include <QtMath>

namespace {

QPointF centerPoint(ComponentModel *component)
{
    return QPointF(component->x(), component->y());
}

QPointF worldToView(qreal worldX, qreal worldY, qreal panX, qreal panY, qreal zoom)
{
    return QPointF(worldX * zoom + panX, worldY * zoom + panY);
}

void connectionEndpointsOnBounding(ComponentModel *sourceComponent,
                                   ComponentModel *targetComponent,
                                   QPointF &sourcePoint, QPointF &targetPoint)
{
    const QPointF srcCenter = centerPoint(sourceComponent);
    const QPointF tgtCenter = centerPoint(targetComponent);
    const qreal dx = tgtCenter.x() - srcCenter.x();
    const qreal dy = tgtCenter.y() - srcCenter.y();

    const qreal srcHalfW = sourceComponent->width() / 2.0;
    const qreal srcHalfH = sourceComponent->height() / 2.0;
    const qreal tgtHalfW = targetComponent->width() / 2.0;
    const qreal tgtHalfH = targetComponent->height() / 2.0;

    const QPointF srcLeft(srcCenter.x() - srcHalfW, srcCenter.y());
    const QPointF srcRight(srcCenter.x() + srcHalfW, srcCenter.y());
    const QPointF srcTop(srcCenter.x(), srcCenter.y() - srcHalfH);
    const QPointF srcBottom(srcCenter.x(), srcCenter.y() + srcHalfH);

    const QPointF tgtLeft(tgtCenter.x() - tgtHalfW, tgtCenter.y());
    const QPointF tgtRight(tgtCenter.x() + tgtHalfW, tgtCenter.y());
    const QPointF tgtTop(tgtCenter.x(), tgtCenter.y() - tgtHalfH);
    const QPointF tgtBottom(tgtCenter.x(), tgtCenter.y() + tgtHalfH);

    if (qAbs(dx) >= qAbs(dy)) {
        sourcePoint = dx >= 0 ? srcRight : srcLeft;
        targetPoint = dx >= 0 ? tgtLeft : tgtRight;
    } else {
        sourcePoint = dy >= 0 ? srcBottom : srcTop;
        targetPoint = dy >= 0 ? tgtTop : tgtBottom;
    }
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

} // namespace

GraphViewportItem::GraphViewportItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    // Phase 2: viewport can render grid/edges with scene graph nodes.
    setFlag(ItemHasContents, true);
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

    m_graph = value;
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

QObject *GraphViewportItem::hitTestComponentAtView(qreal viewX, qreal viewY)
{
    ensureSpatialIndex();
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

                const qreal distSq = distanceToSegmentSquared(worldPoint,
                                                              entry.sourceWorld,
                                                              entry.targetWorld);
                if (distSq <= tolSq && distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestConnection = entry.connection;
                }
            }
        }
    }

    return bestConnection;
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
    //   │   └── m_selectedEdgesGeomNode (world-space, selected edge)
    //   └── m_tempEdgeGeomNode      (screen-space, drag-in-progress edge)
    // -----------------------------------------------------------------------
    if (!oldNode) {
        m_rootNode = new QSGNode();

        m_gridGeomNode = createEmptyLineNode(QColor(QStringLiteral("#e0e0e0")), 1.0f);
        m_rootNode->appendChildNode(m_gridGeomNode);

        m_edgesTransformNode    = new QSGTransformNode();
        m_normalEdgesGeomNode   = createEmptyLineNode(QColor(QStringLiteral("#607d8b")), 2.0f);
        m_selectedEdgesGeomNode = createEmptyLineNode(QColor(QStringLiteral("#ff5722")), 3.0f);
        m_edgesTransformNode->appendChildNode(m_normalEdgesGeomNode);
        m_edgesTransformNode->appendChildNode(m_selectedEdgesGeomNode);
        m_rootNode->appendChildNode(m_edgesTransformNode);

        m_tempEdgeGeomNode = createEmptyLineNode(QColor(QStringLiteral("#90caf9")), 2.0f);
        m_rootNode->appendChildNode(m_tempEdgeGeomNode);

        // Force a full update on the first frame.
        m_graphDirty  = true;
        m_cameraDirty = true;
        oldNode = m_rootNode;
    }

    if (width() <= 0 || height() <= 0)
        return oldNode;

    // -----------------------------------------------------------------------
    // Graph dirty: rebuild edge/temp geometry in world space (O(N+E)).
    // Happens only when topology or selection changes — NOT on every pan frame.
    // -----------------------------------------------------------------------
    if (m_graphDirty) {
        updateEdgesGeometry();
        updateTempEdgeGeometry();
        m_graphDirty = false;
    }

    // -----------------------------------------------------------------------
    // Camera dirty: update grid (screen-space) + edge camera matrix.
    // One 4×4 matrix write replaces O(E) vertex position recalculation.
    // -----------------------------------------------------------------------
    if (m_cameraDirty) {
        updateGridGeometry();

        QMatrix4x4 cam;
        cam.setToIdentity();
        cam.translate(float(m_panX), float(m_panY));
        cam.scale(float(m_zoom));
        m_edgesTransformNode->setMatrix(cam);

        m_cameraDirty = false;
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

    if (!m_renderEdges) {
        clearNode(m_normalEdgesGeomNode);
        clearNode(m_selectedEdgesGeomNode);
        return;
    }

    auto *graphModel = qobject_cast<GraphModel *>(m_graph);
    if (!graphModel) {
        clearNode(m_normalEdgesGeomNode);
        clearNode(m_selectedEdgesGeomNode);
        return;
    }

    const auto &components = graphModel->componentList();
    const auto &connections = graphModel->connectionList();

    // O(N): build id→pointer map to avoid O(N) componentById() per edge.
    QHash<QString, ComponentModel *> componentById;
    componentById.reserve(components.size());
    for (ComponentModel *c : components)
        componentById.insert(c->id(), c);

    // Pre-count vertices (2 per valid connection) to allocate exact buffers.
    int normalCount = 0, selectedCount = 0;
    for (ConnectionModel *conn : connections) {
        if (!componentById.contains(conn->sourceId()) ||
            !componentById.contains(conn->targetId()))
            continue;
        if (static_cast<QObject *>(conn) == m_selectedConnection)
            ++selectedCount;
        else
            ++normalCount;
    }

    // Reallocate only if the edge count changed.
    if (m_normalEdgesGeomNode->geometry()->vertexCount() != normalCount * 2)
        m_normalEdgesGeomNode->geometry()->allocate(normalCount * 2);
    if (m_selectedEdgesGeomNode->geometry()->vertexCount() != selectedCount * 2)
        m_selectedEdgesGeomNode->geometry()->allocate(selectedCount * 2);

    auto *normalV   = m_normalEdgesGeomNode->geometry()->vertexDataAsPoint2D();
    auto *selectedV = m_selectedEdgesGeomNode->geometry()->vertexDataAsPoint2D();
    int nIdx = 0, sIdx = 0;

    for (ConnectionModel *conn : connections) {
        ComponentModel *src = componentById.value(conn->sourceId(), nullptr);
        ComponentModel *tgt = componentById.value(conn->targetId(), nullptr);
        if (!src || !tgt)
            continue;

        QPointF srcWorld, tgtWorld;
        connectionEndpointsOnBounding(src, tgt, srcWorld, tgtWorld);

        // World-space coordinates — QSGTransformNode handles world→screen.
        if (static_cast<QObject *>(conn) == m_selectedConnection) {
            selectedV[sIdx++].set(float(srcWorld.x()), float(srcWorld.y()));
            selectedV[sIdx++].set(float(tgtWorld.x()), float(tgtWorld.y()));
        } else {
            normalV[nIdx++].set(float(srcWorld.x()), float(srcWorld.y()));
            normalV[nIdx++].set(float(tgtWorld.x()), float(tgtWorld.y()));
        }
    }

    m_normalEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
    m_selectedEdgesGeomNode->markDirty(QSGNode::DirtyGeometry);
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

void GraphViewportItem::markSpatialIndexDirty()
{
    m_spatialIndexDirty = true;
}

void GraphViewportItem::ensureSpatialIndex()
{
    if (!m_spatialIndexDirty)
        return;
    rebuildSpatialIndex();
}

void GraphViewportItem::clearComponentGeometryConnections()
{
    for (const QMetaObject::Connection &conn : m_componentGeometryChangedConns)
        QObject::disconnect(conn);
    m_componentGeometryChangedConns.clear();
}

void GraphViewportItem::rebuildSpatialIndex()
{
    m_indexedComponents.clear();
    m_indexedConnections.clear();
    m_componentCellToIndices.clear();
    m_connectionCellToIndices.clear();
    clearComponentGeometryConnections();

    auto *graphModel = qobject_cast<GraphModel *>(m_graph);
    if (!graphModel) {
        m_spatialIndexDirty = false;
        return;
    }

    const auto &components = graphModel->componentList();
    m_indexedComponents.reserve(components.size());

    for (ComponentModel *component : components) {
        if (!component)
            continue;

        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::xChanged,
                    this, &GraphViewportItem::markSpatialIndexDirty));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::yChanged,
                    this, &GraphViewportItem::markSpatialIndexDirty));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::widthChanged,
                    this, &GraphViewportItem::markSpatialIndexDirty));
        m_componentGeometryChangedConns.append(
            connect(component, &ComponentModel::heightChanged,
                    this, &GraphViewportItem::markSpatialIndexDirty));

        const QRectF rect(component->x() - component->width() / 2.0,
                          component->y() - component->height() / 2.0,
                          component->width(),
                          component->height());

        const int idx = m_indexedComponents.size();
        m_indexedComponents.push_back(IndexedComponent { component, rect });

        const QRect cells = cellRangeForRect(rect, m_spatialCellSize);
        for (int cy = cells.top(); cy <= cells.bottom(); ++cy) {
            for (int cx = cells.left(); cx <= cells.right(); ++cx)
                m_componentCellToIndices[cellKey(cx, cy)].append(idx);
        }
    }

    const auto &connections = graphModel->connectionList();
    m_indexedConnections.reserve(connections.size());

    QHash<QString, ComponentModel *> componentById;
    componentById.reserve(components.size());
    for (ComponentModel *component : components) {
        if (component)
            componentById.insert(component->id(), component);
    }

    for (ConnectionModel *connection : connections) {
        if (!connection)
            continue;

        ComponentModel *src = componentById.value(connection->sourceId(), nullptr);
        ComponentModel *tgt = componentById.value(connection->targetId(), nullptr);
        if (!src || !tgt)
            continue;

        QPointF srcWorld;
        QPointF tgtWorld;
        connectionEndpointsOnBounding(src, tgt, srcWorld, tgtWorld);

        const QRectF bounds(QPointF(qMin(srcWorld.x(), tgtWorld.x()),
                                    qMin(srcWorld.y(), tgtWorld.y())),
                            QPointF(qMax(srcWorld.x(), tgtWorld.x()),
                                    qMax(srcWorld.y(), tgtWorld.y())));

        const int idx = m_indexedConnections.size();
        m_indexedConnections.push_back(IndexedConnection {
            connection,
            srcWorld,
            tgtWorld,
            bounds
        });

        const QRect cells = cellRangeForRect(bounds, m_spatialCellSize);
        for (int cy = cells.top(); cy <= cells.bottom(); ++cy) {
            for (int cx = cells.left(); cx <= cells.right(); ++cx)
                m_connectionCellToIndices[cellKey(cx, cy)].append(idx);
        }
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

