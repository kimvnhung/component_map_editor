#include "systempropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaOptionsSource;

const QString kTokenKeyHint =
    QStringLiteral("Token options are sourced from connection token keys, execution-state keys, and schema key defaults.");

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.system");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
              "component/system/error_handler",
              {
                  customize::property_schemas::makeField("errorKey", SchemaFieldType::String, "Error Key", true, QStringLiteral("error"), SchemaFieldWidget::TextField, SchemaFieldSection::Context, 1,
                                                         kTokenKeyHint),
                  customize::property_schemas::makeField("message", SchemaFieldType::String, "Fallback Message", true, QStringLiteral("Unhandled workflow error."), SchemaFieldWidget::TextArea, SchemaFieldSection::Behavior, 2)
              });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
{
    static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString SystemPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList SystemPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList SystemPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}
