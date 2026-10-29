#include "workflowpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include <utils/PropertySchemaHelper.h>

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

const QString kTokenRefHint = QStringLiteral(
    "Choose an exact incoming token field reference. If no ref is selected, the fallback value below is used.");

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.workflow");
    bundle.set_schema_version("1.0.0");

    cme::helper::property_schemas::addTarget(&bundle, "component/workflow/sqrt_graph",
                                             {

                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .defaultValue("Sqrt Graph")
                                                     .textField()
                                                     .section(SchemaFieldSection::General)
                                                     .order(1)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("sRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("S Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(cme::runtime::Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("S")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback S")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(&bundle, "component/workflow/right_triangle_longest_edge",
                                             {
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .defaultValue("Right Triangle Longest Edge")
                                                     .textField()
                                                     .section(SchemaFieldSection::General)
                                                     .order(1)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("sideARef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Side A Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(cme::runtime::Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("sideBRef")
                                                     .type(SchemaFieldType::String)
                                                     .title("Side B Ref")
                                                     .defaultValue(QString())
                                                     .dropdown(cme::runtime::Options::tokenRef())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(3)
                                                     .hint(kTokenRefHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("a")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback A")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("b")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Fallback B")
                                                     .defaultValue(0)
                                                     .spinBox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(21)
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

QString WorkflowPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList WorkflowPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList WorkflowPropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}