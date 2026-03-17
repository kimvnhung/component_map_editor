#include "GridRenderPass.h"

#include <QSGNode>

#include <cmath>

namespace GridRenderPass {
namespace {

qreal normalizedGridStep(qreal baseGridStep,
                         qreal minGridPixelStep,
                         qreal maxGridPixelStep,
                         qreal zoom)
{
    qreal step = baseGridStep * zoom;
    if (step <= 0.0)
        return 0.0;

    while (step < minGridPixelStep)
        step *= 2.0;
    while (step > maxGridPixelStep)
        step /= 2.0;
    return step;
}

qreal positiveModulo(qreal value, qreal modulus)
{
    if (modulus == 0.0)
        return 0.0;
    return std::fmod(std::fmod(value, modulus) + modulus, modulus);
}

} // namespace

void updateGridGeometry(QSGGeometryNode *gridGeomNode,
                        bool renderGrid,
                        qreal width,
                        qreal height,
                        qreal panX,
                        qreal panY,
                        qreal zoom,
                        qreal baseGridStep,
                        qreal minGridPixelStep,
                        qreal maxGridPixelStep)
{
    if (!gridGeomNode)
        return;

    auto *geom = gridGeomNode->geometry();

    if (!renderGrid || width <= 0 || height <= 0) {
        if (geom->vertexCount() > 0) {
            geom->allocate(0);
            gridGeomNode->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    const qreal step = normalizedGridStep(baseGridStep,
                                          minGridPixelStep,
                                          maxGridPixelStep,
                                          zoom);
    if (step <= 0.0) {
        if (geom->vertexCount() > 0) {
            geom->allocate(0);
            gridGeomNode->markDirty(QSGNode::DirtyGeometry);
        }
        return;
    }

    const qreal offsetX = positiveModulo(panX, step);
    const qreal offsetY = positiveModulo(panY, step);

    int verts = 0;
    for (qreal gx = -step + offsetX; gx < width + step; gx += step)
        verts += 2;
    for (qreal gy = -step + offsetY; gy < height + step; gy += step)
        verts += 2;

    if (geom->vertexCount() != verts)
        geom->allocate(verts);

    auto *v = geom->vertexDataAsPoint2D();
    int idx = 0;
    for (qreal gx = -step + offsetX; gx < width + step; gx += step) {
        v[idx++].set(float(gx), 0.0f);
        v[idx++].set(float(gx), float(height));
    }
    for (qreal gy = -step + offsetY; gy < height + step; gy += step) {
        v[idx++].set(0.0f, float(gy));
        v[idx++].set(float(width), float(gy));
    }

    gridGeomNode->markDirty(QSGNode::DirtyGeometry);
}

} // namespace GridRenderPass
