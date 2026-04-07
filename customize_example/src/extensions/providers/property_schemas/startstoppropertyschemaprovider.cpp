#include "startstoppropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "propertyschematemplateutils.h"

namespace {

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.startStop");
    bundle.set_schema_version("1.0.0");

    customize::property_schemas::addTarget(&bundle,
              "component/start",
              {
                  customize::property_schemas::makeField("inputNumber", "number", "Input Number", true, 0, "spinbox", "Behavior", 20,
                            QStringLiteral("Seed number consumed by the start component when simulation begins."),
                            QVariantMap{{QStringLiteral("min"), -1000000}, {QStringLiteral("max"), 1000000}}),
              });

    customize::property_schemas::addTarget(&bundle,
              "component/stop",
              {
                  customize::property_schemas::makeField("title", "string", "Title", true, QString(), "textfield", "Identity", 1),
              });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
{
    static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString StartStopPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList StartStopPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList StartStopPropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}
