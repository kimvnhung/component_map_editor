#include "customizepropertyschemaprovider.h"

#include "property_schemas/connectionpropertyschemaprovider.h"
#include "property_schemas/controlpropertyschemaprovider.h"
#include "property_schemas/mathpropertyschemaprovider.h"
#include "property_schemas/startstoppropertyschemaprovider.h"
#include "property_schemas/systempropertyschemaprovider.h"
#include "property_schemas/workflowpropertyschemaprovider.h"

CustomizePropertySchemaProvider::CustomizePropertySchemaProvider()
{
    m_subProviders.emplace_back(std::make_unique<StartStopPropertySchemaProvider>());
    m_subProviders.emplace_back(std::make_unique<ControlPropertySchemaProvider>());
    m_subProviders.emplace_back(std::make_unique<MathPropertySchemaProvider>());
    m_subProviders.emplace_back(std::make_unique<WorkflowPropertySchemaProvider>());
    m_subProviders.emplace_back(std::make_unique<SystemPropertySchemaProvider>());
    m_subProviders.emplace_back(std::make_unique<ConnectionPropertySchemaProvider>());
}

QString CustomizePropertySchemaProvider::providerId() const
{
    return QStringLiteral("customize.workflow.propertySchema");
}

QStringList CustomizePropertySchemaProvider::schemaTargets() const
{
    QStringList result;
    for (const auto &provider : m_subProviders)
        result.append(provider->schemaTargets());
    result.removeDuplicates();
    return result;
}

QVariantList CustomizePropertySchemaProvider::propertySchema(const QString &targetId) const
{
    for (const auto &provider : m_subProviders) {
        const QVariantList schema = provider->propertySchema(targetId);
        if (!schema.isEmpty())
            return schema;
    }
    return {};
}