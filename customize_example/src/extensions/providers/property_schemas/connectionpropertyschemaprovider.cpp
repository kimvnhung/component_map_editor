#include "connectionpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include <utils/PropertySchemaHelper.h>

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.connection");
    bundle.set_schema_version("1.0.0");

    const QVariantList sideOptions{
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Auto")}, {QStringLiteral("value"), -1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Top")}, {QStringLiteral("value"), 0}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Right")}, {QStringLiteral("value"), 1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Bottom")}, {QStringLiteral("value"), 2}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Left")}, {QStringLiteral("value"), 3}}};

    cme::helper::property_schemas::addTarget(
        &bundle, "connection/flow",
        {
            cme::helper::property_schemas::Field("label")
                .type(SchemaFieldType::String)
                .title("Label")
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(0)
                .build(),
            cme::helper::property_schemas::Field("id")
                .type(SchemaFieldType::String)
                .title("Connection ID")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(1)
                .build(),
            cme::helper::property_schemas::Field("sourceId")
                .type(SchemaFieldType::String)
                .title("Source Component ID")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(2)
                .build(),
            cme::helper::property_schemas::Field("targetId")
                .type(SchemaFieldType::String)
                .title("Target Component ID")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(3)
                .build(),
            cme::helper::property_schemas::Field("sourceSide")
                .type(SchemaFieldType::Enum)
                .title("Source Side")
                .required()
                .defaultValue(-1)
                .dropdown(cme::runtime::Options::custom(sideOptions))
                .section(SchemaFieldSection::Routing)
                .order(20)
                .build(),
            cme::helper::property_schemas::Field("targetSide")
                .type(SchemaFieldType::Enum)
                .title("Target Side")
                .required()
                .defaultValue(-1)
                .dropdown(cme::runtime::Options::custom(sideOptions))
                .section(SchemaFieldSection::Routing)
                .order(21)
                .build(),
            cme::helper::property_schemas::Field("tokenKey")
                .type(SchemaFieldType::String)
                .title("Token Key")
                .defaultValue(QString())
                .dropdown(cme::runtime::Options::tokenRef())
                .section(SchemaFieldSection::Routing)
                .order(22)
                .hint(QStringLiteral("Routing token key used for connection payload selection."))
                .build(),
        });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle& schemaBundle()
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

QVariantList ConnectionPropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}
