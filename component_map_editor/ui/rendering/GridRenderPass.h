#ifndef GRIDRENDERPASS_H
#define GRIDRENDERPASS_H

#include <QSGGeometryNode>

namespace GridRenderPass {

void updateGridGeometry(QSGGeometryNode *gridGeomNode,
                        bool renderGrid,
                        qreal width,
                        qreal height,
                        qreal panX,
                        qreal panY,
                        qreal zoom,
                        qreal baseGridStep,
                        qreal minGridPixelStep,
                        qreal maxGridPixelStep);

} // namespace GridRenderPass

#endif // GRIDRENDERPASS_H
