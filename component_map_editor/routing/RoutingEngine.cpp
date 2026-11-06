#include "RoutingEngine.h"

void RoutingEngine::setStrategy(std::unique_ptr<IRouteStrategy> strategy)
{
    m_strategy = std::move(strategy);
}

QVector<QPointF> RoutingEngine::compute(const RouteRequest &request,
                                        const RoutingContext &context) const
{
    if (!m_strategy)
        return {};

    return m_strategy->compute(request, context);
}
