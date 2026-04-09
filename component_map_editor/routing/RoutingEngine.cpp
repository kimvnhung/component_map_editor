#include "RoutingEngine.h"

LegacyCompositeRouteStrategy::LegacyCompositeRouteStrategy(LegacyRouteFunction legacyRouteFunction)
    : m_legacyRouteFunction(std::move(legacyRouteFunction))
{}

QVector<QPointF> LegacyCompositeRouteStrategy::compute(const RouteRequest &request,
                                                       const RoutingContext &context) const
{
    if (!m_legacyRouteFunction || !request.connection || !request.sourceComponent
        || !request.targetComponent || !context.components) {
        return {};
    }

    return m_legacyRouteFunction(request.connection,
                                 request.sourceComponent,
                                 request.targetComponent,
                                 *context.components,
                                 request.routeMeta);
}

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
