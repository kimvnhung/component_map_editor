#include "component_map_runtime/RuntimeHandlerRegistry.h"

namespace cme::runtime
{

bool RuntimeHandlerRegistry::registerFactory(const QString &componentType, Factory factory)
{
    if (componentType.isEmpty() || !factory || m_factories.contains(componentType))
        return false;

    m_factories.insert(componentType, std::move(factory));
    return true;
}

bool RuntimeHandlerRegistry::registerExtension(const RuntimeExtensionPack &extension, QString *error)
{
    if (extension.extensionId.isEmpty() || extension.version.isEmpty())
    {
        if (error) *error = QStringLiteral("Extension ID and version are required.");
        return false;
    }
    for (const auto &component : extension.components)
    {
        if (component.componentType.isEmpty() || component.handlerId.isEmpty()
            || component.version.isEmpty() || component.stateSchemaVersion < 1)
        {
            if (error) *error = QStringLiteral("Invalid runtime component definition.");
            return false;
        }
        if (m_factories.contains(component.componentType)
            || !extension.factories.contains(component.componentType)
            || !extension.factories.value(component.componentType))
        {
            if (error) *error = QStringLiteral("Duplicate or missing factory for component type: %1")
                                      .arg(component.componentType);
            return false;
        }
    }
    for (const auto &component : extension.components)
    {
        m_factories.insert(component.componentType, extension.factories.value(component.componentType));
        m_definitions.insert(component.componentType, component);
        m_extensions.insert(component.componentType, extension.extensionId);
    }
    return true;
}

bool RuntimeHandlerRegistry::contains(const QString &componentType) const
{
    return m_factories.contains(componentType);
}

std::unique_ptr<RuntimeHandler> RuntimeHandlerRegistry::create(const QString &componentType) const
{
    const auto it = m_factories.constFind(componentType);
    return it == m_factories.cend() ? nullptr : it.value()();
}

QStringList RuntimeHandlerRegistry::componentTypes() const
{
    return m_factories.keys();
}

RuntimeComponentDefinition RuntimeHandlerRegistry::definition(const QString &componentType) const
{
    return m_definitions.value(componentType);
}

QString RuntimeHandlerRegistry::extensionFor(const QString &componentType) const
{
    return m_extensions.value(componentType);
}

} // namespace cme::runtime
