#include "extensionpackbuilder.h"

#include <QString>
#include <base_log.h>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IExtensionPack.h"
#include "extensions/sample_pack/SampleComponentTypeProvider.h"
#include "extensions/sample_pack/SamplePropertySchemaProvider.h"
#include "extensions/sample_pack/SampleExecutionSemanticsProvider.h"



// Internal simple IExtensionPack implementation that owns provider instances
// created by the builder's factories and registers them on demand.
class BuiltExtensionPack : public IExtensionPack
{
public:
    BuiltExtensionPack(std::unique_ptr<IComponentTypeProvider> component,
                       std::unique_ptr<IPropertySchemaProvider> propertySchema,
                       std::unique_ptr<IExecutionSemanticsProvider> execution)
        : m_componentProvider(std::move(component))
        , m_propertySchemaProvider(std::move(propertySchema))
        , m_executionSemanticsProvider(std::move(execution))
    {}

    bool registerProviders(ExtensionContractRegistry &registry, QString *error = nullptr) override
    {
        bool success = true;
        QString componentError, propertyError, executionError;

        if (m_componentProvider)
        {
            success &= registry.registerComponentTypeProvider(m_componentProvider.get(), &componentError);
        }
        else
        {
            LOGW("No component type provider supplied by extension pack");
        }

        if (m_propertySchemaProvider)
        {
            success &= registry.registerPropertySchemaProvider(m_propertySchemaProvider.get(), &propertyError);
        }
        else
        {
            LOGW("No property schema provider supplied by extension pack");
        }

        if (m_executionSemanticsProvider)
        {
            success &= registry.registerExecutionSemanticsProvider(m_executionSemanticsProvider.get(), &executionError);
        }
        else
        {
            LOGW("No execution semantics provider supplied by extension pack");
        }

        if (!success && error)
        {
            *error = "Failed to register providers:";

            if (!componentError.isEmpty())
            {
                *error += "\nComponentTypeProvider: " + componentError;
            }

            if (!propertyError.isEmpty())
            {
                *error += "\nPropertySchemaProvider: " + propertyError;
            }

            if (!executionError.isEmpty())
            {
                *error += "\nExecutionSemanticsProvider: " + executionError;
            }
        }

        return success;
    }

private:
    std::unique_ptr<IComponentTypeProvider> m_componentProvider;
    std::unique_ptr<IPropertySchemaProvider> m_propertySchemaProvider;
    std::unique_ptr<IExecutionSemanticsProvider> m_executionSemanticsProvider;
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

ExtensionPackBuilder &ExtensionPackBuilder::withPropertySchemaRegistryFactory(PropertySchemaRegistryFactory f)
{
    m_propertySchemaRegistryFactory = std::move(f);
    return *this;
}

ExtensionPackBuilder &ExtensionPackBuilder::withExecutionSandboxFactory(ExecutionSandboxFactory f)
{
    m_executionSandboxFactory = std::move(f);
    return *this;
}

PackFactory ExtensionPackBuilder::build() const
{
    // Return a factory that constructs an IExtensionPack with providers created by the builder's factories.
    auto componentProvider = m_componentFactory ? m_componentFactory() :
                             std::make_unique<SampleComponentTypeProvider>();
    auto propertySchemaProvider = m_propertySchemaFactory ? m_propertySchemaFactory() :
                                  std::make_unique<SamplePropertySchemaProvider>();
    auto executionSemanticsProvider = m_executionFactory ? m_executionFactory() :
                                      std::make_unique<SampleExecutionSemanticsProvider>();

    return utils::makeFactory<BuiltExtensionPack>(
               std::move(componentProvider),
               std::move(propertySchemaProvider),
               std::move(executionSemanticsProvider));
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
