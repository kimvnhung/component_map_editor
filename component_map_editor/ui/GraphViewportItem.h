#ifndef GRAPHVIEWPORTITEM_H
#define GRAPHVIEWPORTITEM_H

#include <QHash>
#include <QPointer>
#include <QPointF>
#include <QRectF>
#include <QQuickItem>
#include <QQmlEngine>
#include <QMutex>
#include <QVariantMap>
#include <QVector>
#include <atomic>

class QSGGeometryNode;
class QSGNode;
class QSGSimpleTextureNode;
class QSGTransformNode;
class ComponentModel;
class ConnectionModel;
class QSGTexture;

// Phase 1 skeleton item for future C++ viewport rendering.
// This class intentionally provides camera API only (panX/panY/zoom) and
// coordinate conversion helpers; it does not render content.
class GraphViewportItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject *graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(qreal panX READ panX WRITE setPanX NOTIFY panXChanged FINAL)
    Q_PROPERTY(qreal panY READ panY WRITE setPanY NOTIFY panYChanged FINAL)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged FINAL)

    Q_PROPERTY(bool renderGrid READ renderGrid WRITE setRenderGrid NOTIFY renderGridChanged FINAL)
    Q_PROPERTY(bool renderEdges READ renderEdges WRITE setRenderEdges NOTIFY renderEdgesChanged FINAL)
    Q_PROPERTY(bool renderNodes READ renderNodes WRITE setRenderNodes NOTIFY renderNodesChanged FINAL)

    Q_PROPERTY(qreal baseGridStep READ baseGridStep WRITE setBaseGridStep NOTIFY baseGridStepChanged FINAL)
    Q_PROPERTY(qreal minGridPixelStep READ minGridPixelStep WRITE setMinGridPixelStep NOTIFY minGridPixelStepChanged FINAL)
    Q_PROPERTY(qreal maxGridPixelStep READ maxGridPixelStep WRITE setMaxGridPixelStep NOTIFY maxGridPixelStepChanged FINAL)

    Q_PROPERTY(QObject *selectedConnection READ selectedConnection WRITE setSelectedConnection NOTIFY selectedConnectionChanged FINAL)
    Q_PROPERTY(QObject *selectedComponent READ selectedComponent WRITE setSelectedComponent NOTIFY selectedComponentChanged FINAL)
    Q_PROPERTY(bool tempConnectionDragging READ tempConnectionDragging WRITE setTempConnectionDragging NOTIFY tempConnectionDraggingChanged FINAL)
    Q_PROPERTY(QPointF tempStart READ tempStart WRITE setTempStart NOTIFY tempStartChanged FINAL)
    Q_PROPERTY(QPointF tempEnd READ tempEnd WRITE setTempEnd NOTIFY tempEndChanged FINAL)

public:
    explicit GraphViewportItem(QQuickItem *parent = nullptr);

    QObject *graph() const;
    void setGraph(QObject *value);

    qreal panX() const;
    void setPanX(qreal value);

    qreal panY() const;
    void setPanY(qreal value);

    qreal zoom() const;
    void setZoom(qreal value);

    bool renderGrid() const;
    void setRenderGrid(bool value);

    bool renderEdges() const;
    void setRenderEdges(bool value);

    bool renderNodes() const;
    void setRenderNodes(bool value);

    qreal baseGridStep() const;
    void setBaseGridStep(qreal value);

    qreal minGridPixelStep() const;
    void setMinGridPixelStep(qreal value);

    qreal maxGridPixelStep() const;
    void setMaxGridPixelStep(qreal value);

    QObject *selectedConnection() const;
    void setSelectedConnection(QObject *value);

    QObject *selectedComponent() const;
    void setSelectedComponent(QObject *value);

    bool tempConnectionDragging() const;
    void setTempConnectionDragging(bool value);

    QPointF tempStart() const;
    void setTempStart(const QPointF &value);

    QPointF tempEnd() const;
    void setTempEnd(const QPointF &value);

    // world -> view using current camera values.
    Q_INVOKABLE QPointF worldToView(qreal worldX, qreal worldY) const;

    // view -> world using current camera values.
    Q_INVOKABLE QPointF viewToWorld(qreal viewX, qreal viewY) const;

    // Computes camera state for zoom-at-cursor anchored at a view-space point.
    Q_INVOKABLE QVariantMap zoomAtViewAnchor(qreal viewX, qreal viewY,
                                             qreal zoomFactor,
                                             qreal minZoom,
                                             qreal maxZoom,
                                             qreal epsilon = 0.000001) const;

    // Requests scene graph rebuild on next frame.
    Q_INVOKABLE void repaint();

    // Phase 3: indexed hit-testing in C++.
    Q_INVOKABLE QObject *hitTestComponentAtView(qreal viewX, qreal viewY);
    Q_INVOKABLE QObject *hitTestConnectionAtView(qreal viewX, qreal viewY,
                                                 qreal tolerancePx = 8.0);

    // Phase 4 observability and lifecycle hooks.
    Q_INVOKABLE void purgeLabelCache();
    Q_INVOKABLE int labelTextureCacheSize() const;
    Q_INVOKABLE qreal rendererMemoryEstimateMb() const;

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

signals:
    void graphChanged();
    void panXChanged();
    void panYChanged();
    void zoomChanged();
    void renderGridChanged();
    void renderEdgesChanged();
    void renderNodesChanged();
    void baseGridStepChanged();
    void minGridPixelStepChanged();
    void maxGridPixelStepChanged();
    void selectedConnectionChanged();
    void selectedComponentChanged();
    void tempConnectionDraggingChanged();
    void tempStartChanged();
    void tempEndChanged();

private:
    struct IndexedComponent {
        QPointer<ComponentModel> component;
        QRectF worldRect;
    };

    struct IndexedConnection {
        QPointer<ConnectionModel> connection;
        QPointF sourceWorld;
        QPointF targetWorld;
        QRectF worldBounds;
    };

    qreal normalizedGridStep() const;
    static qreal positiveModulo(qreal value, qreal modulus);
    void requestGraphRebuild();
    void scheduleGraphRebuild();
    void executeScheduledGraphRebuild();
    void updateLodState();
    void requestNodeRepaint();

    void markSpatialIndexDirty();
    void ensureSpatialIndex();
    void rebuildSpatialIndex();
    void clearComponentGeometryConnections();
    void clearConnectionGeometryConnections();
    QVector<int> visibleComponentIndices() const;
    QVector<IndexedComponent> visibleComponentsSnapshot() const;
    void updateNodeGeometry();
    void updateLabelNodes();
    QString labelCacheKey(const ComponentModel *component) const;
    void clearLabelTexturesOnRenderThread();

    static quint64 cellKey(int cx, int cy);
    static QRect cellRangeForRect(const QRectF &rect, qreal cellSize);
    static qreal distanceToSegmentSquared(const QPointF &point,
                                          const QPointF &a,
                                          const QPointF &b);

    // Called from updatePaintNode (render thread during sync).
    void updateGridGeometry();
    void updateEdgesGeometry();
    void updateTempEdgeGeometry();

    QObject *m_graph = nullptr;
    qreal m_panX = 0.0;
    qreal m_panY = 0.0;
    qreal m_zoom = 1.0;

    bool m_renderGrid = false;
    bool m_renderEdges = false;
    bool m_renderNodes = false;

    qreal m_baseGridStep = 30.0;
    qreal m_minGridPixelStep = 16.0;
    qreal m_maxGridPixelStep = 96.0;

    QObject *m_selectedConnection = nullptr;
    QObject *m_selectedComponent = nullptr;
    bool m_tempConnectionDragging = false;
    QPointF m_tempStart;
    QPointF m_tempEnd;

    QMetaObject::Connection m_componentsChangedConn;
    QMetaObject::Connection m_connectionsChangedConn;
    QVector<QMetaObject::Connection> m_componentGeometryChangedConns;
    QVector<QMetaObject::Connection> m_connectionGeometryChangedConns;

    // Persistent scene-graph node cache (render-thread only).
    QSGNode          *m_rootNode              = nullptr;
    QSGGeometryNode  *m_gridGeomNode          = nullptr;
    QSGNode          *m_nodesRootNode         = nullptr;
    QSGTransformNode *m_nodesTransformNode    = nullptr;
    QSGGeometryNode  *m_nodeFillGeomNode      = nullptr;
    QSGGeometryNode  *m_nodeOutlineGeomNode   = nullptr;
    QSGNode          *m_nodeLabelsRootNode    = nullptr;
    QSGTransformNode *m_edgesTransformNode    = nullptr;
    QSGGeometryNode  *m_normalEdgesGeomNode   = nullptr;
    QSGGeometryNode  *m_selectedEdgesGeomNode = nullptr;
    QSGGeometryNode  *m_tempEdgeGeomNode      = nullptr;

    // Dirty flags: written on main thread, read on render thread (sync phase).
    bool m_graphDirty  = true;   // edge/temp geometry must be rebuilt
    bool m_cameraDirty = true;   // grid geometry + edge transform matrix must update
    bool m_nodeDirty   = true;   // node fill/outline and label state must update
    bool m_graphRebuildScheduled = false;

    // Phase 6 LOD flags (updated from camera zoom).
    bool m_lodSimpleEdges = false;
    bool m_lodHideNodeLabels = false;
    bool m_lodHideNodeOutlines = false;

    // Spatial index (main thread only; used by QML hit-test invokables).
    bool m_spatialIndexDirty = true;
    qreal m_spatialCellSize = 300.0;
    QVector<IndexedComponent> m_indexedComponents;
    QVector<IndexedConnection> m_indexedConnections;
    QHash<quint64, QVector<int>> m_componentCellToIndices;
    QHash<quint64, QVector<int>> m_connectionCellToIndices;
    mutable QMutex m_spatialIndexMutex;

    QHash<QString, QSGTexture *> m_labelTextureCache;
    QVector<QSGSimpleTextureNode *> m_labelNodes;
    bool m_labelCachePurgeRequested = false;
    std::atomic<int> m_labelTextureCacheCount{0};
    std::atomic<qint64> m_labelTextureCacheBytes{0};
};

#endif // GRAPHVIEWPORTITEM_H
