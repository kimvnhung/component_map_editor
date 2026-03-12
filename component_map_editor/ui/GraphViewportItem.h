#ifndef GRAPHVIEWPORTITEM_H
#define GRAPHVIEWPORTITEM_H

#include <QPointF>
#include <QQuickItem>
#include <QQmlEngine>

class QSGGeometryNode;
class QSGTransformNode;

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

    Q_PROPERTY(qreal baseGridStep READ baseGridStep WRITE setBaseGridStep NOTIFY baseGridStepChanged FINAL)
    Q_PROPERTY(qreal minGridPixelStep READ minGridPixelStep WRITE setMinGridPixelStep NOTIFY minGridPixelStepChanged FINAL)
    Q_PROPERTY(qreal maxGridPixelStep READ maxGridPixelStep WRITE setMaxGridPixelStep NOTIFY maxGridPixelStepChanged FINAL)

    Q_PROPERTY(QObject *selectedConnection READ selectedConnection WRITE setSelectedConnection NOTIFY selectedConnectionChanged FINAL)
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

    qreal baseGridStep() const;
    void setBaseGridStep(qreal value);

    qreal minGridPixelStep() const;
    void setMinGridPixelStep(qreal value);

    qreal maxGridPixelStep() const;
    void setMaxGridPixelStep(qreal value);

    QObject *selectedConnection() const;
    void setSelectedConnection(QObject *value);

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

    // Requests scene graph rebuild on next frame.
    Q_INVOKABLE void repaint();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

signals:
    void graphChanged();
    void panXChanged();
    void panYChanged();
    void zoomChanged();
    void renderGridChanged();
    void renderEdgesChanged();
    void baseGridStepChanged();
    void minGridPixelStepChanged();
    void maxGridPixelStepChanged();
    void selectedConnectionChanged();
    void tempConnectionDraggingChanged();
    void tempStartChanged();
    void tempEndChanged();

private:
    qreal normalizedGridStep() const;
    static qreal positiveModulo(qreal value, qreal modulus);
    void requestGraphRebuild();

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

    qreal m_baseGridStep = 30.0;
    qreal m_minGridPixelStep = 16.0;
    qreal m_maxGridPixelStep = 96.0;

    QObject *m_selectedConnection = nullptr;
    bool m_tempConnectionDragging = false;
    QPointF m_tempStart;
    QPointF m_tempEnd;

    QMetaObject::Connection m_componentsChangedConn;
    QMetaObject::Connection m_connectionsChangedConn;

    // Persistent scene-graph node cache (render-thread only).
    QSGNode          *m_rootNode              = nullptr;
    QSGGeometryNode  *m_gridGeomNode          = nullptr;
    QSGTransformNode *m_edgesTransformNode    = nullptr;
    QSGGeometryNode  *m_normalEdgesGeomNode   = nullptr;
    QSGGeometryNode  *m_selectedEdgesGeomNode = nullptr;
    QSGGeometryNode  *m_tempEdgeGeomNode      = nullptr;

    // Dirty flags: written on main thread, read on render thread (sync phase).
    bool m_graphDirty  = true;   // edge/temp geometry must be rebuilt
    bool m_cameraDirty = true;   // grid geometry + edge transform matrix must update
};

#endif // GRAPHVIEWPORTITEM_H
