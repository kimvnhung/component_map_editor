#include "GraphPropertySchemaProvider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "utils/PropertySchemaHelper.h"

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("builtin.propertySchema.graph");
    bundle.set_schema_version("1.0.0");

    cme::helper::property_schemas::addTarget(
        &bundle, "graph",
        {
            cme::helper::property_schemas::makeField("id", SchemaFieldType::String, "Graph ID", true, QString(),
                                                     SchemaFieldWidget::TextArea, SchemaFieldSection::Identity, 0),
            cme::helper::property_schemas::makeField("title", SchemaFieldType::String, "Title", true, "Unitled",
                                                     SchemaFieldWidget::TextField, SchemaFieldSection::Identity, 2),
            cme::helper::property_schemas::makeField("start_component_id", SchemaFieldType::String,
                                                     "Compnent Startup ID", true, QString(),
                                                     SchemaFieldWidget::TextField, SchemaFieldSection::Behavior, 10),
            cme::helper::property_schemas::makeField("start_component_title", SchemaFieldType::String,
                                                     "Compnent Startup Title", true, "Unitled",
                                                     SchemaFieldWidget::TextArea, SchemaFieldSection::Behavior, 11),

        });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle& schemaBundle()
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

QVariantList GraphPropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}
