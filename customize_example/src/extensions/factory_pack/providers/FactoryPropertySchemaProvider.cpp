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

        customize::property_schemas::addTarget(&bundle,
                                               FactoryComponentTypeProvider::TypeFruitProducer,
        {
            customize::property_schemas::makeField("id", SchemaFieldType::String, "Component ID", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 0),
            customize::property_schemas::makeField("price", SchemaFieldType::Number, "Fruite Price", true, 1, SchemaFieldWidget::SpinBox, SchemaFieldSection::Behavior, 20,
                                                   QStringLiteral("Seed number consumed by the start component when simulation begins."),
            QVariantMap{{QStringLiteral("min"), -1000000}, {QStringLiteral("max"), 1000000}}),
        });

        customize::property_schemas::addTarget(&bundle,
                                               FactoryComponentTypeProvider::TypeStore,
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
        });

        customize::property_schemas::addTarget(&bundle,
                                               FactoryComponentTypeProvider::TypeEmployee,
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
        });

        customize::property_schemas::addTarget(&bundle,
                                               FactoryComponentTypeProvider::TypeSeller,
        {
            customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 1),
        });

        customize::property_schemas::addTarget(&bundle,
                                               FactoryComponentTypeProvider::TypeManager,
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