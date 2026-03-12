#include "GraphViewportItem.h"

#include "ComponentModel.h"
#include "ConnectionModel.h"
#include "GraphModel.h"

#include <QColor>
#include <QHash>
#include <QMatrix4x4>
#include <QSGFlatColorMaterial>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGTransformNode>
#include <algorithm>
#include <cmath>
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

void GraphViewportItem::repaint()
{
    m_cameraDirty = true;
    update();
}

void GraphViewportItem::requestGraphRebuild()
{
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

