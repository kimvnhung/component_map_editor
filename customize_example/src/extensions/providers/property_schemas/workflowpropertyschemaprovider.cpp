#include "workflowpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

const QString kTokenRefHint =
    QStringLiteral("Choose an exact incoming token field reference. If no ref is selected, the fallback value below is used.");

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.workflow");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
                                           "component/workflow/sqrt_graph",
    {
        customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", false, "Sqrt Graph", SchemaFieldWidget::TextField, SchemaFieldSection::General, 1),
        customize::property_schemas::makeField("sRef", SchemaFieldType::String, "S Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                               kTokenRefHint,
                                               {}, {}, {},
                                               SchemaOptionsSource::TokenKeyOptions),
        customize::property_schemas::makeField("S", SchemaFieldType::Number, "Fallback S", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20)
    });

    customize::property_schemas::addTarget(&bundle,
                                           "component/workflow/right_triangle_longest_edge",
    {
        customize::property_schemas::makeField("title", SchemaFieldType::String, "Title", false, "Right Triangle Longest Edge", SchemaFieldWidget::TextField, SchemaFieldSection::General, 1),
        customize::property_schemas::makeField("sideARef", SchemaFieldType::String, "Side A Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 2,
                                               kTokenRefHint,
                                               {}, {}, {},
                                               SchemaOptionsSource::TokenKeyOptions),
        customize::property_schemas::makeField("sideBRef", SchemaFieldType::String, "Side B Ref", false, QString(), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 3,
                                               kTokenRefHint,
                                               {}, {}, {},
                                               SchemaOptionsSource::TokenKeyOptions),
        customize::property_schemas::makeField("a", SchemaFieldType::Number, "Fallback A", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 20),
        customize::property_schemas::makeField("b", SchemaFieldType::Number, "Fallback B", false, 0, SchemaFieldWidget::SpinBox, SchemaFieldSection::Fallback, 21)
    });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
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

QVariantList WorkflowPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}