#include "controlpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

const QString kTokenKeyHint =
    QStringLiteral("Token options are sourced from connection token keys, execution-state keys, and schema key defaults.");

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.control");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
              "component/control/loop",
              {
                  customize::property_schemas::makeField("iterKey", SchemaFieldType::String, "Iter Key", true, QStringLiteral("iter"), SchemaFieldWidget::Dropdown, "Context", 1,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("maxIterKey", SchemaFieldType::String, "Max Iter Key", true, QStringLiteral("maxIter"), SchemaFieldWidget::Dropdown, "Context", 2,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("continueKey", SchemaFieldType::String, "Continue Key", true, QStringLiteral("continueLoop"), SchemaFieldWidget::Dropdown, "Context", 3,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("conditionKey", SchemaFieldType::String, "Condition Key", true, QStringLiteral("condition"), SchemaFieldWidget::Dropdown, "Context", 4,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("iter", SchemaFieldType::Number, "Fallback Iter", false, 0, SchemaFieldWidget::SpinBox, "Fallback", 20),
                  customize::property_schemas::makeField("maxIter", SchemaFieldType::Number, "Fallback Max Iter", false, 10, SchemaFieldWidget::SpinBox, "Fallback", 21),
                  customize::property_schemas::makeField("condition", SchemaFieldType::Boolean, "Fallback Condition", false, true, SchemaFieldWidget::Checkbox, "Fallback", 22)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/control/ifelse",
              {
                  customize::property_schemas::makeField("conditionKey", SchemaFieldType::String, "Condition Key", true, QStringLiteral("condition"), SchemaFieldWidget::Dropdown, "Context", 1,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("trueRouteKey", SchemaFieldType::String, "True Route Key", true, QStringLiteral("routeTrue"), SchemaFieldWidget::Dropdown, "Context", 2,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("falseRouteKey", SchemaFieldType::String, "False Route Key", true, QStringLiteral("routeFalse"), SchemaFieldWidget::Dropdown, "Context", 3,
                                                         kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
                  customize::property_schemas::makeField("condition", SchemaFieldType::Boolean, "Fallback Condition", false, false, SchemaFieldWidget::Checkbox, "Fallback", 20)
              });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
{
    static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString ControlPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList ControlPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList ControlPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}
