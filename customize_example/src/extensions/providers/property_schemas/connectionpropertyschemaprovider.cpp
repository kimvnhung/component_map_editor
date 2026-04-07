#include "connectionpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.connection");
    bundle.set_schema_version("1.0.0");

    const QVariantList sideOptions{
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Auto")},   {QStringLiteral("value"), -1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Top")},    {QStringLiteral("value"),  0}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Right")},  {QStringLiteral("value"),  1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Bottom")}, {QStringLiteral("value"),  2}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Left")},   {QStringLiteral("value"),  3}}
    };

    customize::property_schemas::addTarget(&bundle,
              "connection/flow",
              {
                  customize::property_schemas::makeField("label", SchemaFieldType::String, "Label", false, QString(), SchemaFieldWidget::TextField, "Identity", 0),
                  customize::property_schemas::makeField("id", SchemaFieldType::String, "Connection ID", true, QString(), SchemaFieldWidget::TextField, "Identity", 1),
                  customize::property_schemas::makeField("sourceId", SchemaFieldType::String, "Source Component ID", true, QString(), SchemaFieldWidget::TextField, "Identity", 2),
                  customize::property_schemas::makeField("targetId", SchemaFieldType::String, "Target Component ID", true, QString(), SchemaFieldWidget::TextField, "Identity", 3),
                  customize::property_schemas::makeField("sourceSide", SchemaFieldType::Enum, "Source Side", true, -1, SchemaFieldWidget::Dropdown, "Routing", 20,
                            QString(), {}, {}, sideOptions),
                  customize::property_schemas::makeField("targetSide", SchemaFieldType::Enum, "Target Side", true, -1, SchemaFieldWidget::Dropdown, "Routing", 21,
                            QString(), {}, {}, sideOptions),
                  customize::property_schemas::makeField("tokenKey", SchemaFieldType::String, "Token Key", false, QString(), SchemaFieldWidget::Dropdown, "Routing", 22,
                            QStringLiteral("Routing token key used for connection payload selection."),
                            {}, {}, {},
                            SchemaOptionsSource::TokenKeys)
              });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
{
    static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString ConnectionPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList ConnectionPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList ConnectionPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}
