#include "FactoryPropertySchemaProvider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "extensions/providers/property_schemas/propertyschematemplateutils.h"
#include "FactoryComponentTypeProvider.h"

namespace
{

    using cme::runtime::SchemaFieldType;
    using cme::runtime::SchemaFieldWidget;
    using cme::runtime::SchemaFieldSection;


    cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
    {
        cme::templates::v1::PropertySchemaTemplateBundle bundle;
        bundle.set_provider_id("factory.propertySchema");
        bundle.set_schema_version("1.0.0");

        // TODO: targetId must start with component for matching template on PropertyPanel
        // This need to be created a machanic for exposing error or something for ensure devs know about this!
        customize::property_schemas::addTarget(&bundle,
                                               "component/factory/fruit_producer",
        {
            customize::property_schemas::makeField("id", SchemaFieldType::String, "Component ID", true, QString(), SchemaFieldWidget::TextArea, SchemaFieldSection::Identity, 0),
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Component Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
            customize::property_schemas::makeField("price", SchemaFieldType::Number, "Fruite Price", true, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Behavior, 20,
                                                   QStringLiteral("Seed number consumed by the start component when simulation begins."),
            QVariantMap{{QStringLiteral("min"), -1000000}, {QStringLiteral("max"), 1000000}}),
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/factory/store",
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/factory/employee",
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/factory/seller",
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
        });

        customize::property_schemas::addTarget(&bundle,
                                               "component/factory/manager",
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
        });

        return bundle;
    }

    const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
    {
        static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
        return kBundle;
    }

}

FactoryPropertySchemaProvider::FactoryPropertySchemaProvider()
{
}

QString FactoryPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList FactoryPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList FactoryPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
               schemaBundle(), targetId);
}