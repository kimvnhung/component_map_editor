#include "component_map_runtime/RuntimeHandlerRegistry.h"

namespace cme::runtime
{

bool RuntimeHandlerRegistry::registerFactory(const QString &componentType, Factory factory)
{
    if (componentType.isEmpty() || !factory || m_factories.contains(componentType))
    {
        return false;
    }

    m_factories.insert(componentType, std::move(factory));
    return true;
}

bool RuntimeHandlerRegistry::contains(const QString &componentType) const
{
    return m_factories.contains(componentType);
}

std::unique_ptr<RuntimeHandler> RuntimeHandlerRegistry::create(const QString &componentType) const
{
    const auto it = m_factories.constFind(componentType);
    if (it == m_factories.cend())
    {
        return nullptr;
    }

    return it.value()();
}

QStringList RuntimeHandlerRegistry::componentTypes() const
{
    return m_factories.keys();
}

} // namespace cme::runtime
