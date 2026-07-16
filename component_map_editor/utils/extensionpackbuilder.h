#ifndef EXTENSIONPACKBUILDER_H
#define EXTENSIONPACKBUILDER_H

#include "utils/common.h"

// Builder that produces an ExtensionStartupLoader::PackFactory which
// constructs an IExtensionPack that registers the provided provider instances.
class ExtensionPackBuilder
{
public:
    ExtensionPackBuilder() = default;

    ExtensionPackBuilder &withComponentProviderFactory(ComponentFactory f);
    ExtensionPackBuilder &withPropertySchemaProviderFactory(PropertySchemaFactory f);
    ExtensionPackBuilder &withExecutionSemanticsFactory(ExecutionSemanticsFactory f);
    ExtensionPackBuilder &withConnectionPolicyProviderFactory(ConnectionPolicyFactory f);
    ExtensionPackBuilder &withValidationProviderFactory(ValidationFactory f);
    ExtensionPackBuilder &withActionProviderFactory(ActionFactory f);

    // Setup manifest
    ExtensionPackBuilder &withManifest(const ExtensionManifest &manifest);
    ExtensionPackBuilder &withExtensionId(const QString &id);
    ExtensionPackBuilder &withDisplayName(const QString &name);
    ExtensionPackBuilder &withExtensionVersion(const QString &version);
    ExtensionPackBuilder &withMinCoreApi(int major, int minor, int patch);
    ExtensionPackBuilder &withMaxCoreApi(int major, int minor, int patch);
    ExtensionPackBuilder &withCapabilities(const QStringList &capabilities);

    // Returns a PackFactory compatible with ExtensionStartupLoader::registerFactory
    PackFactoryEntry build() const;
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
