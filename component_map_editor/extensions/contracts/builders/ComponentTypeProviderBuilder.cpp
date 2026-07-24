#include "ComponentTypeProviderBuilder.h"

#include "extensions/runtime/templates/ComponentTypeTemplateAdapter.h"
#include <provider_templates.pb.h>

class ComponentTypeProviderBuilder::BuiltComponentTypeProvider : public IComponentTypeProvider
{
    // IComponentTypeProvider interface
public:
    QString providerId() const override
    {
        return cme::runtime::templates::ComponentTypeTemplateAdapter::providerId(m_bundle);
    }
    QStringList componentTypeIds() const override
    {
        return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeIds(m_bundle);
    }
    QVariantMap componentTypeDescriptor(const QString &componentTypeId) const override
    {
        return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeDescriptor(m_bundle, componentTypeId);
    }
    QVariantMap defaultComponentProperties(const QString &componentTypeId) const override
    {
        return cme::runtime::templates::ComponentTypeTemplateAdapter::defaultComponentProperties(m_bundle, componentTypeId);
    }

    void addComponentTypeTemplate(const cme::templates::v1::ComponentTypeTemplate &type)
    {
        *m_bundle.add_component_types() = type;
    }

    void addComponentTypeDefaultsTemplate(const cme::templates::v1::ComponentTypeDefaultPropertiesTemplate &defaults)
    {
        *m_bundle.add_defaults() = defaults;
    }

    void setProviderId(const QString &providerId)
    {
        m_bundle.set_provider_id(providerId.toStdString());
    }

    void setSchemaVersion(const QString &schemaVersion)
    {
        m_bundle.set_schema_version(schemaVersion.toStdString());
    }
private:
    cme::templates::v1::ComponentTypeTemplateBundle m_bundle;
};

ComponentTypeProviderBuilder::ComponentTypeProviderBuilder()
    : m_builtProvider(std::make_unique<BuiltComponentTypeProvider>())
{
}

ComponentTypeProviderBuilder::~ComponentTypeProviderBuilder() = default;

ComponentTypeProviderBuilder &ComponentTypeProviderBuilder::withComponentTypeProviderFactory(ComponentFactory f)
{
    auto provider = f();

    if (!provider)
    {
        throw std::runtime_error("ComponentFactory returned nullptr.");
    }

    auto builtProvider = dynamic_cast<BuiltComponentTypeProvider *>(provider.get());

    if (!builtProvider)
    {
        throw std::runtime_error("ComponentFactory did not return a BuiltComponentTypeProvider.");
    }

    m_builtProvider = std::unique_ptr<BuiltComponentTypeProvider>(builtProvider);
    return *this;
}

ComponentFactory ComponentTypeProviderBuilder::build() const
{
    return [provider = m_builtProvider.get()]() -> std::unique_ptr<IComponentTypeProvider>
    {
        return std::unique_ptr<IComponentTypeProvider>(new BuiltComponentTypeProvider(*provider));
    };
}
