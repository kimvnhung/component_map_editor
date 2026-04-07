#include "mathpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.math");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
              "component/math/add",
              {
                  customize::property_schemas::makeTokenKeyField("inputAKey", "Input A Key", true, QStringLiteral("a"), "Context", 1),
                  customize::property_schemas::makeTokenKeyField("inputBKey", "Input B Key", true, QStringLiteral("b"), "Context", 2),
                  customize::property_schemas::makeTokenKeyField("outputKey", "Output Key", true, QStringLiteral("sum"), "Context", 3),
                  customize::property_schemas::makeField("a", "number", "Fallback A", false, 0, "spinbox", "Fallback", 20),
                  customize::property_schemas::makeField("b", "number", "Fallback B", false, 0, "spinbox", "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/subtract",
              {
                  customize::property_schemas::makeTokenKeyField("inputAKey", "Input A Key", true, QStringLiteral("a"), "Context", 1),
                  customize::property_schemas::makeTokenKeyField("inputBKey", "Input B Key", true, QStringLiteral("b"), "Context", 2),
                  customize::property_schemas::makeTokenKeyField("outputKey", "Output Key", true, QStringLiteral("difference"), "Context", 3),
                  customize::property_schemas::makeTokenKeyField("errorKey", "Error Key", true, QStringLiteral("error"), "Context", 4),
                  customize::property_schemas::makeField("a", "number", "Fallback A", false, 0, "spinbox", "Fallback", 20),
                  customize::property_schemas::makeField("b", "number", "Fallback B", false, 0, "spinbox", "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/multiply",
              {
                  customize::property_schemas::makeTokenKeyField("inputAKey", "Input A Key", true, QStringLiteral("a"), "Context", 1),
                  customize::property_schemas::makeTokenKeyField("inputBKey", "Input B Key", true, QStringLiteral("b"), "Context", 2),
                  customize::property_schemas::makeTokenKeyField("outputKey", "Output Key", true, QStringLiteral("product"), "Context", 3),
                  customize::property_schemas::makeTokenKeyField("errorKey", "Error Key", true, QStringLiteral("error"), "Context", 4),
                  customize::property_schemas::makeField("a", "number", "Fallback A", false, 1, "spinbox", "Fallback", 20),
                  customize::property_schemas::makeField("b", "number", "Fallback B", false, 1, "spinbox", "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/divide",
              {
                  customize::property_schemas::makeTokenKeyField("inputAKey", "Input A Key", true, QStringLiteral("a"), "Context", 1),
                  customize::property_schemas::makeTokenKeyField("inputBKey", "Input B Key", true, QStringLiteral("b"), "Context", 2),
                  customize::property_schemas::makeTokenKeyField("outputKey", "Output Key", true, QStringLiteral("quotient"), "Context", 3),
                  customize::property_schemas::makeTokenKeyField("errorKey", "Error Key", true, QStringLiteral("error"), "Context", 4),
                  customize::property_schemas::makeField("a", "number", "Fallback A", false, 1, "spinbox", "Fallback", 20),
                  customize::property_schemas::makeField("b", "number", "Fallback B", false, 1, "spinbox", "Fallback", 21)
              });

    customize::property_schemas::addTarget(&bundle,
              "component/math/mod",
              {
                  customize::property_schemas::makeTokenKeyField("inputAKey", "Input A Key", true, QStringLiteral("a"), "Context", 1),
                  customize::property_schemas::makeTokenKeyField("inputBKey", "Input B Key", true, QStringLiteral("b"), "Context", 2),
                  customize::property_schemas::makeTokenKeyField("outputKey", "Output Key", true, QStringLiteral("result"), "Context", 3),
                  customize::property_schemas::makeTokenKeyField("errorKey", "Error Key", true, QStringLiteral("error"), "Context", 4),
                  customize::property_schemas::makeField("a", "number", "Fallback A", false, 1, "spinbox", "Fallback", 20),
                  customize::property_schemas::makeField("b", "number", "Fallback B", false, 1, "spinbox", "Fallback", 21)
              });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
{
    static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString MathPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList MathPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList MathPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}
