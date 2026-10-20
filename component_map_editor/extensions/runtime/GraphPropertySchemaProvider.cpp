#include "GraphPropertySchemaProvider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "utils/PropertySchemaHelper.h"

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("builtin.propertySchema.graph");
    bundle.set_schema_version("1.0.0");

    cme::helper::property_schemas::addTarget(&bundle, "graph",
                                             {
                                                 cme::helper::property_schemas::Field("id")
                                                     .type(SchemaFieldType::String)
                                                     .title("Graph ID")
                                                     .required()
                                                     .defaultValue(QString())
                                                     .textArea()
                                                     .section(SchemaFieldSection::Identity)
                                                     .order(0)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .required()
                                                     .defaultValue("Untitled")
                                                     .textField()
                                                     .section(SchemaFieldSection::Identity)
                                                     .order(2)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("start_component_id")
                                                     .type(SchemaFieldType::String)
                                                     .title("Component Startup ID")
                                                     .required()
                                                     .defaultValue(QString())
                                                     .dropdown(cme::runtime::Options::defined("components"))
                                                     .section(SchemaFieldSection::Behavior)
                                                     .order(10)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("start_component_title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Component Startup Title")
                                                     .required()
                                                     .defaultValue("Untitled")
                                                     .textArea()
                                                     .section(SchemaFieldSection::Behavior)
                                                     .order(11)
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
