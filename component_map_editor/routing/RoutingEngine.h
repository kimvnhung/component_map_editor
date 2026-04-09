#ifndef ROUTINGENGINE_H
#define ROUTINGENGINE_H

#include <QVector>

#include <functional>
#include <memory>

#include "RouteTypes.h"

class IRouteStrategy
{
public:
    virtual ~IRouteStrategy() = default;
    virtual QVector<QPointF> compute(const RouteRequest &request,
                                     const RoutingContext &context) const = 0;
};

class LegacyCompositeRouteStrategy : public IRouteStrategy
{
public:
    using LegacyRouteFunction =
        std::function<QVector<QPointF>(ConnectionModel *connection,
                                       ComponentModel *sourceComponent,
                                       ComponentModel *targetComponent,
                                       const QList<ComponentModel *> &components,
                                       const ConnectionRouteMeta *routeMeta)>;

    explicit LegacyCompositeRouteStrategy(LegacyRouteFunction legacyRouteFunction);

    QVector<QPointF> compute(const RouteRequest &request,
                             const RoutingContext &context) const override;

private:
    LegacyRouteFunction m_legacyRouteFunction;
};

class RoutingEngine
{
public:
    void setStrategy(std::unique_ptr<IRouteStrategy> strategy);

    QVector<QPointF> compute(const RouteRequest &request,
                             const RoutingContext &context) const;

private:
    std::unique_ptr<IRouteStrategy> m_strategy;
};

#endif // ROUTINGENGINE_H
