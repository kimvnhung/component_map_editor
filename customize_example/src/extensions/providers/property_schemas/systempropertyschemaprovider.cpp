#include "systempropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.system");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
              "component/system/error_handler",
              {
                  customize::property_schemas::makeTokenKeyField("errorKey", "Error Key", true, QStringLiteral("error"), "Context", 1),
                  customize::property_schemas::makeField("message", SchemaFieldType::String, "Fallback Message", true, QStringLiteral("Unhandled workflow error."), SchemaFieldWidget::TextArea, "Behavior", 2)
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
