#include "extensionpackbuilder.h"

#include <QString>
#include <base_log.h>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IExtensionPack.h"
#include "extensions/sample_pack/SampleComponentTypeProvider.h"
#include "extensions/sample_pack/SamplePropertySchemaProvider.h"
#include "extensions/sample_pack/SampleExecutionSemanticsProvider.h"
#include "extensions/common.h"



// Internal simple IExtensionPack implementation that owns provider instances
// created by the builder's factories and registers them on demand.
class BuiltExtensionPack : public IExtensionPack
{
public:
    BuiltExtensionPack()
    {
        m_manifest.extensionId      = QStringLiteral("built.extension.pack");
        m_manifest.displayName      = QStringLiteral("Built Extension Pack");
        m_manifest.extensionVersion = QStringLiteral("1.0.0");
        m_manifest.minCoreApi       = { 1, 0, 0 };
        m_manifest.maxCoreApi       = { 1, 99, 99 };
        m_manifest.capabilities     = extensions::Capability_All;
    }

    const ExtensionManifest &manifest() const
    {
        return m_manifest;
    }

    void setExtensionId(const QString &id)
    {
        m_manifest.extensionId = id;
    }

    void setDisplayName(const QString &name)
    {
        m_manifest.displayName = name;
    }

    void setExtensionVersion(const QString &version)
    {
        m_manifest.extensionVersion = version;
    }

    void setMinCoreApi(int major, int minor, int patch)
    {
        m_manifest.minCoreApi = { major, minor, patch };
    }

    void setMaxCoreApi(int major, int minor, int patch)
    {
        m_manifest.maxCoreApi = { major, minor, patch };
    }

    void setCapabilities(const extensions::Capability &capabilities)
    {
        m_manifest.capabilities = capabilities;
    }

    void append(ComponentFactory f) { m_componentFactories.push_back(std::move(f)); }
    void append(PropertySchemaFactory f) { m_propertySchemaFactories.push_back(std::move(f)); }
    void append(ExecutionSemanticsFactory f) { m_executionFactories.push_back(std::move(f)); }
    void append(ConnectionPolicyFactory f) { m_connectionPolicyFactories.push_back(std::move(f)); }
    void append(ValidationFactory f) { m_validationFactories.push_back(std::move(f)); }
    void append(ActionFactory f) { m_actionFactories.push_back(std::move(f)); }

    bool registerProviders(ExtensionContractRegistry &registry, QString *error) override
    {
        // Register the manifest first, then register each provider created by the factories.
        if (!registry.registerManifest(m_manifest, error))
        {
            return false;
        }

        for (const auto &factory : m_componentFactories)
        {
            auto provider = factory();

            if (!provider)
            {
                continue;
            }

            if (!registry.registerComponentTypeProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &factory : m_propertySchemaFactories)
        {
            auto provider = factory();

            if (!provider)
            {
                continue;
            }

            if (!registry.registerPropertySchemaProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &factory : m_executionFactories)
        {
            auto provider = factory();

            if (!provider)
            {
                continue;
            }

            if (!registry.registerExecutionSemanticsProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &factory : m_connectionPolicyFactories)
        {
            auto provider = factory();

            if (!provider)
            {
                continue;
            }

            if (!registry.registerConnectionPolicyProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &factory : m_validationFactories)
        {
            auto provider = factory();

            if (!provider)
            {
                continue;
            }

            if (!registry.registerValidationProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &factory : m_actionFactories)
        {
            auto provider = factory();

            if (!provider)
            {
                continue;
            }

            if (!registry.registerActionProvider(provider.get(), error))
            {
                return false;
            }
        }

        return true;
    }

private:
    ExtensionManifest m_manifest;
    std::vector<ComponentFactory> m_componentFactories;
    std::vector<PropertySchemaFactory> m_propertySchemaFactories;
    std::vector<ExecutionSemanticsFactory> m_executionFactories;
    std::vector<ConnectionPolicyFactory> m_connectionPolicyFactories;
    std::vector<ValidationFactory> m_validationFactories;
    std::vector<ActionFactory> m_actionFactories;

};

ExtensionPackBuilder &ExtensionPackBuilder::withComponentProviderFactory(ComponentFactory f)
{
    m_componentFactory = std::move(f);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withPropertySchemaProviderFactory(PropertySchemaFactory f)
{
    m_propertySchemaFactory = std::move(f);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withExecutionSemanticsFactory(ExecutionSemanticsFactory f)
{
    m_executionFactory = std::move(f);
    return *this;
}

PackFactoryEntry ExtensionPackBuilder::build() const
{
    // Return a factory that constructs an IExtensionPack with providers created by the builder's factories.
    auto componentProvider = m_componentFactory ? m_componentFactory() :
                             std::make_unique<SampleComponentTypeProvider>();
    auto propertySchemaProvider = m_propertySchemaFactory ? m_propertySchemaFactory() :
                                  std::make_unique<SamplePropertySchemaProvider>();
    auto executionSemanticsProvider = m_executionFactory ? m_executionFactory() :
                                      std::make_unique<SampleExecutionSemanticsProvider>();

    return
    {
        "",
        utils::makeFactory<BuiltExtensionPack>()
    };
}

std::unique_ptr<PropertySchemaRegistry> ExtensionPackBuilder::createPropertySchemaRegistry() const
{
    if (m_propertySchemaRegistryFactory)
    {
        return m_propertySchemaRegistryFactory();
    }

    return std::make_unique<PropertySchemaRegistry>();
}

std::unique_ptr<GraphExecutionSandbox> ExtensionPackBuilder::createExecutionSandbox() const
{
    if (m_executionSandboxFactory)
    {
        return m_executionSandboxFactory();
    }

    return std::make_unique<GraphExecutionSandbox>();
}
