#ifndef GRAPHVIEWPORTITEM_H
#define GRAPHVIEWPORTITEM_H

#include <QPointF>
#include <QQuickItem>
#include <QQmlEngine>

// Phase 1 skeleton item for future C++ viewport rendering.
// This class intentionally provides camera API only (panX/panY/zoom) and
// coordinate conversion helpers; it does not render content.
class GraphViewportItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qreal panX READ panX WRITE setPanX NOTIFY panXChanged FINAL)
    Q_PROPERTY(qreal panY READ panY WRITE setPanY NOTIFY panYChanged FINAL)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged FINAL)

public:
    explicit GraphViewportItem(QQuickItem *parent = nullptr);

    qreal panX() const;
    void setPanX(qreal value);

    qreal panY() const;
    void setPanY(qreal value);

    qreal zoom() const;
    void setZoom(qreal value);

    // world -> view using current camera values.
    Q_INVOKABLE QPointF worldToView(qreal worldX, qreal worldY) const;

    // view -> world using current camera values.
    Q_INVOKABLE QPointF viewToWorld(qreal viewX, qreal viewY) const;

signals:
    void panXChanged();
    void panYChanged();
    void zoomChanged();

private:
    qreal m_panX = 0.0;
    qreal m_panY = 0.0;
    qreal m_zoom = 1.0;
};

#endif // GRAPHVIEWPORTITEM_H
