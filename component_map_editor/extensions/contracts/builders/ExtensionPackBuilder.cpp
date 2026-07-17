#include "extensionpackbuilder.h"

#include <QString>
#include <base_log.h>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IExtensionPack.h"
#include "extensions/common.h"
#include "utils/common.h"


using namespace extensions;
// Internal simple IExtensionPack implementation that owns provider instances
// created by the builder's factories and registers them on demand.
class ExtensionPackBuilder::BuiltExtensionPack : public IExtensionPack
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

    BuiltExtensionPack(BuiltExtensionPack &&other) noexcept
        : m_manifest(std::move(other.m_manifest)),
        m_componentProviders(std::move(other.m_componentProviders)),
        m_propertySchemaProviders(std::move(other.m_propertySchemaProviders)),
        m_executionProviders(std::move(other.m_executionProviders)),
        m_connectionPolicyProviders(std::move(other.m_connectionPolicyProviders)),
        m_validationProviders(std::move(other.m_validationProviders)),
        m_actionProviders(std::move(other.m_actionProviders))
    {
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

    void append(ComponentFactory f)
    {
        auto provider = f();

        if (provider)
        {
            m_componentProviders.push_back(std::move(provider));
        }
    }

    void append(PropertySchemaFactory f)
    {
        auto provider = f();

        if (provider)
        {
            m_propertySchemaProviders.push_back(std::move(provider));
        }
    }

    void append(ExecutionSemanticsFactory f)
    {
        auto provider = f();

        if (provider)
        {
            m_executionProviders.push_back(std::move(provider));
        }
    }

    void append(ConnectionPolicyFactory f)
    {
        auto provider = f();

        if (provider)
        {
            m_connectionPolicyProviders.push_back(std::move(provider));
        }
    }

    void append(ValidationFactory f)
    {
        auto provider = f();

        if (provider)
        {
            m_validationProviders.push_back(std::move(provider));
        }
    }

    void append(ActionFactory f)
    {
        auto provider = f();

        if (provider)
        {
            m_actionProviders.push_back(std::move(provider));
        }
    }


    bool registerProviders(ExtensionContractRegistry &registry, QString *error) override
    {
        for (const auto &provider : m_componentProviders)
        {
            if (!registry.registerComponentTypeProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &provider : m_propertySchemaProviders)
        {
            if (!registry.registerPropertySchemaProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &provider : m_executionProviders)
        {
            if (!registry.registerExecutionSemanticsProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &provider : m_connectionPolicyProviders)
        {
            if (!registry.registerConnectionPolicyProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &provider : m_validationProviders)
        {
            if (!registry.registerValidationProvider(provider.get(), error))
            {
                return false;
            }
        }

        for (const auto &provider : m_actionProviders)
        {
            if (!registry.registerActionProvider(provider.get(), error))
            {
                return false;
            }
        }

        return true;
    }

    int componentProvidersCount() const
    {
        return static_cast<int>(m_componentProviders.size());
    }

    int propertySchemaProvidersCount() const
    {
        return static_cast<int>(m_propertySchemaProviders.size());
    }

    int executionProvidersCount() const
    {
        return static_cast<int>(m_executionProviders.size());
    }

    int connectionPolicyProvidersCount() const
    {
        return static_cast<int>(m_connectionPolicyProviders.size());
    }

    int validationProvidersCount() const
    {
        return static_cast<int>(m_validationProviders.size());
    }

    int actionProvidersCount() const
    {
        return static_cast<int>(m_actionProviders.size());
    }

private:
    ExtensionManifest m_manifest;
    std::vector<ComponentProviderPtr> m_componentProviders;
    std::vector<PropertySchemaProviderPtr> m_propertySchemaProviders;
    std::vector<ExecutionSemanticsProviderPtr> m_executionProviders;
    std::vector<ConnectionPolicyProviderPtr> m_connectionPolicyProviders;
    std::vector<ValidationProviderPtr> m_validationProviders;
    std::vector<ActionProviderPtr> m_actionProviders;

};

ExtensionPackBuilder::ExtensionPackBuilder()
    : m_builtPack(std::make_unique<BuiltExtensionPack>())
{
}

ExtensionPackBuilder::~ExtensionPackBuilder() = default;

ExtensionPackBuilder &ExtensionPackBuilder::withComponentProviderFactory(ComponentFactory f)
{
    m_builtPack->append(std::move(f));
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withPropertySchemaProviderFactory(PropertySchemaFactory f)
{
    m_builtPack->append(std::move(f));
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withExecutionSemanticsFactory(ExecutionSemanticsFactory f)
{
    m_builtPack->append(std::move(f));
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withConnectionPolicyProviderFactory(ConnectionPolicyFactory f)
{
    m_builtPack->append(std::move(f));
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withValidationProviderFactory(ValidationFactory f)
{
    m_builtPack->append(std::move(f));
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withActionProviderFactory(ActionFactory f)
{
    m_builtPack->append(std::move(f));
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withManifest(const ExtensionManifest &manifest)
{
    m_builtPack->setExtensionId(manifest.extensionId);
    m_builtPack->setDisplayName(manifest.displayName);
    m_builtPack->setExtensionVersion(manifest.extensionVersion);
    m_builtPack->setMinCoreApi(manifest.minCoreApi.major, manifest.minCoreApi.minor, manifest.minCoreApi.patch);
    m_builtPack->setMaxCoreApi(manifest.maxCoreApi.major, manifest.maxCoreApi.minor, manifest.maxCoreApi.patch);
    m_builtPack->setCapabilities(manifest.capabilities);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withExtensionId(const QString &id)
{
    m_builtPack->setExtensionId(id);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withDisplayName(const QString &name)
{
    m_builtPack->setDisplayName(name);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withExtensionVersion(const QString &version)
{
    m_builtPack->setExtensionVersion(version);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withMinCoreApi(int major, int minor, int patch)
{
    m_builtPack->setMinCoreApi(major, minor, patch);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withMaxCoreApi(int major, int minor, int patch)
{
    m_builtPack->setMaxCoreApi(major, minor, patch);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withCapabilities(const extensions::Capability& capabilities)
{
    m_builtPack->setCapabilities(capabilities);
    return *this;
}

std::unique_ptr<PackFactoryEntry> ExtensionPackBuilder::build() const
{
    if (!m_builtPack)
    {
        throw PackFactoryMissingException();
    }

    // Verify that the capabilities are set correctly in the manifest, and the Provider size is not zero for each capability
    if (hasCapability(Capability_ComponentTypes, m_builtPack->manifest().capabilities) &&
        m_builtPack->componentProvidersCount() == 0)
    {
        throw std::runtime_error("Capability_ComponentTypes is set, but no component providers were added.");
    }

    if (hasCapability(Capability_PropertySchema, m_builtPack->manifest().capabilities) &&
        m_builtPack->propertySchemaProvidersCount() == 0)
    {
        throw std::runtime_error("Capability_PropertySchema is set, but no property schema providers were added.");
    }

    if (hasCapability(Capability_ExecutionSemantics, m_builtPack->manifest().capabilities) &&
        m_builtPack->executionProvidersCount() == 0)
    {
        throw std::runtime_error("Capability_ExecutionSemantics is set, but no execution semantics providers were added.");
    }

    if (hasCapability(Capability_ConnectionPolicy, m_builtPack->manifest().capabilities) &&
        m_builtPack->connectionPolicyProvidersCount() == 0)
    {
        throw std::runtime_error("Capability_ConnectionPolicy is set, but no connection policy providers were added.");
    }

    if (hasCapability(Capability_Validation, m_builtPack->manifest().capabilities) &&
        m_builtPack->validationProvidersCount() == 0)
    {
        throw std::runtime_error("Capability_Validation is set, but no validation providers were added.");
    }

    if (hasCapability(Capability_Actions, m_builtPack->manifest().capabilities) &&
        m_builtPack->actionProvidersCount() == 0)
    {
        throw std::runtime_error("Capability_Actions is set, but no action providers were added.");
    }

    return std::make_unique<PackFactoryEntry>(PackFactoryEntry
                                              {
                                                  m_builtPack->manifest().extensionId,
                                                  utils::makeFactory<BuiltExtensionPack>(std::move(*m_builtPack))
                                              });
}