#include "controlpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.control");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
              "component/control/loop",
              {
                  customize::property_schemas::makeTokenKeyField("iterKey", "Iter Key", true, QStringLiteral("iter"), "Context", 1),
                  customize::property_schemas::makeTokenKeyField("maxIterKey", "Max Iter Key", true, QStringLiteral("maxIter"), "Context", 2),
                  customize::property_schemas::makeTokenKeyField("continueKey", "Continue Key", true, QStringLiteral("continueLoop"), "Context", 3),
                  customize::property_schemas::makeTokenKeyField("conditionKey", "Condition Key", true, QStringLiteral("condition"), "Context", 4),
                  customize::property_schemas::makeField("iter", "number", "Fallback Iter", false, 0, "spinbox", "Fallback", 20),
                  customize::property_schemas::makeField("maxIter", "number", "Fallback Max Iter", false, 10, "spinbox", "Fallback", 21),
                  customize::property_schemas::makeField("condition", "bool", "Fallback Condition", false, true, "checkbox", "Fallback", 22)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/control/ifelse",
              {
                  customize::property_schemas::makeTokenKeyField("conditionKey", "Condition Key", true, QStringLiteral("condition"), "Context", 1),
                  customize::property_schemas::makeTokenKeyField("trueRouteKey", "True Route Key", true, QStringLiteral("routeTrue"), "Context", 2),
                  customize::property_schemas::makeTokenKeyField("falseRouteKey", "False Route Key", true, QStringLiteral("routeFalse"), "Context", 3),
                  customize::property_schemas::makeField("condition", "bool", "Fallback Condition", false, false, "checkbox", "Fallback", 20)
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
