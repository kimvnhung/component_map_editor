#include "GraphPropertySchemaProvider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "utils/PropertySchemaHelper.h"

namespace
{

    using cme::runtime::SchemaFieldType;
    using cme::runtime::SchemaFieldWidget;
    using cme::runtime::SchemaFieldSection;

    cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
    {
        cme::templates::v1::PropertySchemaTemplateBundle bundle;
        bundle.set_provider_id("builtin.propertySchema.graph");
        bundle.set_schema_version("1.0.0");

        cme::helper::property_schemas::addTarget(&bundle,
                "component/graph",
        {
            cme::helper::property_schemas::makeField("id", SchemaFieldType::String, "Component ID", true, QString(), SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 0),
            cme::helper::property_schemas::makeField("inputNumber", SchemaFieldType::Number, "Input Number", true, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Behavior, 20,
                    QStringLiteral("Seed number consumed by the start component when simulation begins."),
            QVariantMap{{QStringLiteral("min"), -1000000}, {QStringLiteral("max"), 1000000}}),
        });

        return bundle;
    }

    const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
    {
        static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
        return kBundle;
    }

} // namespace


QString GraphPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList GraphPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList GraphPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
               schemaBundle(), targetId);
}
