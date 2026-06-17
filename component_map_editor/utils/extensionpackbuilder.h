#ifndef EXTENSIONPACKBUILDER_H
#define EXTENSIONPACKBUILDER_H

#include "utils/common.h"

// Builder that produces an ExtensionStartupLoader::PackFactory which
// constructs an IExtensionPack that registers the provided provider instances.
class ExtensionPackBuilder
{
public:
    ExtensionPackBuilder() = default;

    ExtensionPackBuilder &withComponentProviderFactory(ComponentFactory f) { m_componentFactory = std::move(f); return *this; }
    ExtensionPackBuilder &withPropertySchemaProviderFactory(PropertySchemaFactory f) { m_propertySchemaFactory = std::move(f); return *this; }
    ExtensionPackBuilder &withExecutionSemanticsFactory(ExecutionSemanticsFactory f) { m_executionFactory = std::move(f); return *this; }
    ExtensionPackBuilder &withPropertySchemaRegistryFactory(PropertySchemaRegistryFactory f) { m_propertySchemaRegistryFactory = std::move(f); return *this; }
    ExtensionPackBuilder &withExecutionSandboxFactory(ExecutionSandboxFactory f) { m_executionSandboxFactory = std::move(f); return *this; }

    // Returns a PackFactory compatible with ExtensionStartupLoader::registerFactory
    PackFactory build() const;
    std::unique_ptr<PropertySchemaRegistry> createPropertySchemaRegistry() const;
    std::unique_ptr<GraphExecutionSandbox> createExecutionSandbox() const;

private:
    ComponentFactory m_componentFactory;
    PropertySchemaFactory m_propertySchemaFactory;
    ExecutionSemanticsFactory m_executionFactory;
    PropertySchemaRegistryFactory m_propertySchemaRegistryFactory;
    ExecutionSandboxFactory m_executionSandboxFactory;
};

#endif // EXTENSIONPACKBUILDER_H
