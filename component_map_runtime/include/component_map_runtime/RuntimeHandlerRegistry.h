#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_HANDLER_REGISTRY_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_HANDLER_REGISTRY_H

#include "component_map_runtime/RuntimeTypes.h"

#include <functional>
#include <memory>
#include <QHash>
#include <QStringList>

namespace cme::runtime
{

class RuntimeHandler
{
public:
    virtual ~RuntimeHandler() = default;

    virtual QString componentType() const = 0;
    virtual bool process(const RuntimeEvent &event,
                         QByteArray &state,
                         RuntimeEvent *output,
                         RuntimeFailure *failure) = 0;
};

class RuntimeHandlerRegistry
{
public:
    using Factory = std::function<std::unique_ptr<RuntimeHandler>()>;

    bool registerFactory(const QString &componentType, Factory factory);
    bool contains(const QString &componentType) const;
    std::unique_ptr<RuntimeHandler> create(const QString &componentType) const;
    QStringList componentTypes() const;

private:
    QHash<QString, Factory> m_factories;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_HANDLER_REGISTRY_H
