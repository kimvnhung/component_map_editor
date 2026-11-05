#include "component_map_runtime/RuntimeHandlerRegistry.h"

namespace cme::runtime
{

namespace
{
ComponentDescriptor descriptorFromDefinition(const RuntimeComponentDefinition &definition,
                                             const QString &sourceId)
{
    ComponentDescriptor descriptor;
    descriptor.identity = definition.componentType;
    descriptor.version = definition.version;
    if (definition.hasDescriptor)
    {
        auto descriptor = definition.descriptor;
        descriptor.sourceId = sourceId;
        return descriptor;
    }
    descriptor.sourceId = sourceId;
    descriptor.status = ComponentStatus::Available;
    descriptor.trustState = TrustState::Trusted;
    descriptor.title = definition.componentType;
    descriptor.execution.mode = ExecutionMode::Builtin;
    descriptor.execution.inputRequirements = definition.inputMessageTypes;
    descriptor.execution.outputGuarantees = definition.outputMessageTypes;
    descriptor.execution.errorBehavior = QStringLiteral("structured");
    for (const auto &input : definition.inputMessageTypes)
    {
        descriptor.ports.append({QStringLiteral("in_%1").arg(input), QStringLiteral("input"), input,
                                 QStringLiteral("single"), false, {}, {}});
    }
    for (const auto &output : definition.outputMessageTypes)
    {
        descriptor.ports.append({QStringLiteral("out_%1").arg(output), QStringLiteral("output"), output,
                                 QStringLiteral("single"), false, {}, {}});
    }
    return descriptor;
}
}

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
        m_descriptors.insert(component.componentType, descriptorFromDefinition(component, extension.extensionId));
    }
    return true;
}

bool RuntimeHandlerRegistry::registerDescriptor(const QString &componentType,
                                                 const ComponentDescriptor &descriptor,
                                                 QString *error)
{
    if (componentType.isEmpty() || componentType != descriptor.identity)
    {
        if (error) *error = QStringLiteral("Descriptor identity must match component type.");
        return false;
    }
    const auto errors = validateComponentDescriptor(descriptor);
    if (!errors.isEmpty())
    {
        if (error) *error = errors.constFirst();
        return false;
    }
    if (!m_factories.contains(componentType))
    {
        if (error) *error = QStringLiteral("A factory is required before registering a descriptor.");
        return false;
    }
    if (m_descriptors.contains(componentType))
    {
        if (error) *error = QStringLiteral("A descriptor is already registered for component type.");
        return false;
    }
    m_descriptors.insert(componentType, descriptor);
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

ComponentDescriptor RuntimeHandlerRegistry::descriptor(const QString &componentType) const
{
    return m_descriptors.value(componentType);
}

QStringList RuntimeHandlerRegistry::descriptorDiagnostics(const QString &componentType) const
{
    return descriptor(componentType).diagnostics;
}

bool RuntimeHandlerRegistry::hasDescriptor(const QString &componentType) const
{
    return m_descriptors.contains(componentType);
}

} // namespace cme::runtime
