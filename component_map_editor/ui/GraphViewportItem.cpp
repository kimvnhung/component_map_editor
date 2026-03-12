#include "GraphViewportItem.h"

#include <QtGlobal>

GraphViewportItem::GraphViewportItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    // Phase 1: camera API only, no scene graph nodes yet.
    setFlag(ItemHasContents, false);
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
