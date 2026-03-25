#include "ComponentRenderPass.h"

#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGNode>

namespace ComponentRenderPass {
namespace {

void clearColoredNode(QSGGeometryNode *node)
{
    if (node && node->geometry()->vertexCount() > 0) {
        node->geometry()->allocate(0);
        node->markDirty(QSGNode::DirtyGeometry);
    }
}

void appendTriangleRect(QSGGeometry::ColoredPoint2D *verts,
                        int &idx,
                        const QRectF &rect,
                        const QColor &color)
{
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
}

} // namespace

void updateComponentBodyGeometry(QSGGeometryNode *componentFillGeomNode,
                                  QSGGeometryNode *componentOutlineGeomNode,
                                  bool renderComponents,
                                  bool lodHideComponentOutlines,
                                  const QVector<QRectF> &worldRects,
                                  const QVector<QColor> &fillColors,
                                  const QVector<bool> &selectedFlags)
{
    if (!componentFillGeomNode || !componentOutlineGeomNode)
        return;

    if (!renderComponents) {
        clearColoredNode(componentFillGeomNode);
        clearColoredNode(componentOutlineGeomNode);
        return;
    }

    const int count = worldRects.size();
    if (fillColors.size() != count || selectedFlags.size() != count)
        return;

    const int fillVertexCount = count * 6;
    if (componentFillGeomNode->geometry()->vertexCount() != fillVertexCount)
        componentFillGeomNode->geometry()->allocate(fillVertexCount);

    const int outlineVertexCount = lodHideComponentOutlines ? 0 : count * 24;
    if (componentOutlineGeomNode->geometry()->vertexCount() != outlineVertexCount)
        componentOutlineGeomNode->geometry()->allocate(outlineVertexCount);

    auto *fillVerts = componentFillGeomNode->geometry()->vertexDataAsColoredPoint2D();
    auto *outlineVerts = componentOutlineGeomNode->geometry()->vertexDataAsColoredPoint2D();
    int fillIdx = 0;
    int outlineIdx = 0;

    for (int i = 0; i < count; ++i) {
        const QRectF &viewRect = worldRects.at(i);
        const QColor fillColor = fillColors.at(i);

        appendTriangleRect(fillVerts, fillIdx, viewRect, fillColor);

        if (!lodHideComponentOutlines) {
            const bool selected = selectedFlags.at(i);
            const qreal borderWidth = selected ? 2.5 : 1.5;
            const QColor borderColor = selected
                ? QColor(QStringLiteral("#ff5722"))
                : fillColor.darker(140);

            appendTriangleRect(outlineVerts,
                               outlineIdx,
                               QRectF(viewRect.left(), viewRect.top(), viewRect.width(), borderWidth),
                               borderColor);
            appendTriangleRect(outlineVerts,
                               outlineIdx,
                               QRectF(viewRect.left(), viewRect.bottom() - borderWidth, viewRect.width(), borderWidth),
                               borderColor);
            appendTriangleRect(outlineVerts,
                               outlineIdx,
                               QRectF(viewRect.left(),
                                      viewRect.top() + borderWidth,
                                      borderWidth,
                                      qMax<qreal>(0.0, viewRect.height() - borderWidth * 2.0)),
                               borderColor);
            appendTriangleRect(outlineVerts,
                               outlineIdx,
                               QRectF(viewRect.right() - borderWidth,
                                      viewRect.top() + borderWidth,
                                      borderWidth,
                                      qMax<qreal>(0.0, viewRect.height() - borderWidth * 2.0)),
                               borderColor);
        }
    }

    componentFillGeomNode->markDirty(QSGNode::DirtyGeometry);
    componentOutlineGeomNode->markDirty(QSGNode::DirtyGeometry);
}

} // namespace ComponentRenderPass
