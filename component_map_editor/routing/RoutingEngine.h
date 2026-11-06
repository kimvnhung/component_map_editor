#ifndef ROUTINGENGINE_H
#define ROUTINGENGINE_H

#include <QVector>

#include <memory>

#include "RouteTypes.h"

class IRouteStrategy
{
public:
    virtual ~IRouteStrategy() = default;
    virtual QVector<QPointF> compute(const RouteRequest &request,
                                     const RoutingContext &context) const = 0;
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
