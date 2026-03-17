#ifndef ORTHOGONALHEURISTICSTRATEGY_H
#define ORTHOGONALHEURISTICSTRATEGY_H

#include "RoutingEngine.h"

// Concrete routing strategy that computes orthogonal (axis-aligned) routes
// using a multi-candidate heuristic search followed by a grid-based A* fallback.
// This strategy was extracted from GraphViewportItem to allow independent testing
// and future replacement with alternative algorithms.
class OrthogonalHeuristicStrategy : public IRouteStrategy
{
public:
    QVector<QPointF> compute(const RouteRequest &request,
                             const RoutingContext &context) const override;
};

#endif // ORTHOGONALHEURISTICSTRATEGY_H
