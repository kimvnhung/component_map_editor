#include "FactoryPropertySchemaProvider.h"

#include <utils/PropertySchemaHelper.h>

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "FactoryComponentTypeProvider.h"

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("factory.propertySchema");
    bundle.set_schema_version("1.0.0");

    // TODO: targetId must start with component for matching template on PropertyPanel
    // This need to be created a machanic for exposing error or something for ensure devs know about this!
    cme::helper::property_schemas::addTarget(
        &bundle, "component/factory/fruit_producer",
        {
            cme::helper::property_schemas::Field("id")
                .type(SchemaFieldType::String)
                .title("Component ID")
                .required()
                .defaultValue(QString())
                .textArea()
                .section(SchemaFieldSection::Identity)
                .order(0)
                .build(),
            cme::helper::property_schemas::Field("title")
                .type(SchemaFieldType::String)
                .title("Component Title")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(1)
                .build(),
            cme::helper::property_schemas::Field("price")
                .type(SchemaFieldType::Number)
                .title("Fruite Price")
                .required()
                .defaultValue(1)
                .spinBox(-1000000, 1000000)
                .section(SchemaFieldSection::Behavior)
                .order(20)
                .hint(QStringLiteral("Seed number consumed by the start component when simulation begins."))
                .build(),
        });

    cme::helper::property_schemas::addTarget(&bundle, "component/factory/store",
                                             {
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .required()
                                                     .defaultValue(QString())
                                                     .textField()
                                                     .section(SchemaFieldSection::Identity)
                                                     .order(1)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/factory/employee",
                                             {
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .required()
                                                     .defaultValue(QString())
                                                     .textField()
                                                     .section(SchemaFieldSection::Identity)
                                                     .order(1)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/factory/seller",
                                             {
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .required()
                                                     .defaultValue(QString())
                                                     .textField()
                                                     .section(SchemaFieldSection::Identity)
                                                     .order(1)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(
        &bundle, "component/factory/manager",
        {
            cme::helper::property_schemas::Field("title")
                .type(SchemaFieldType::String)
                .title("Title")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(1)
                .build(),
            cme::helper::property_schemas::Field("capital")
                .type(SchemaFieldType::Number)
                .title("Capital")
                .required()
                .defaultValue(0)
                .spinBox(0, 1000000)
                .section(SchemaFieldSection::Behavior)
                .order(100)
                .hint(QStringLiteral("Current capital available for the manager to make purchases."))
                .build(),
            cme::helper::property_schemas::Field("buyAmount")
                .type(SchemaFieldType::Number)
                .title("Buy Amount")
                .required()
                .defaultValue(10)
                .spinBox(1, 1000000)
                .section(SchemaFieldSection::Behavior)
                .order(110)
                .hint(QStringLiteral("The amount of capital required for the manager to make a purchase."))
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

FactoryPropertySchemaProvider::FactoryPropertySchemaProvider() {}

QString FactoryPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList FactoryPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList FactoryPropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}