#include "startstoppropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include <utils/PropertySchemaHelper.h>

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.startStop");
    bundle.set_schema_version("1.0.0");

    cme::helper::property_schemas::addTarget(
        &bundle, "component/start",
        {
            cme::helper::property_schemas::Field("id")
                .type(SchemaFieldType::String)
                .title("Component ID")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(0)
                .build(),
            cme::helper::property_schemas::Field("inputNumber")
                .type(SchemaFieldType::Number)
                .title("Input Number")
                .required()
                .defaultValue(0)
                .spinBox(-1000000, 1000000)
                .section(SchemaFieldSection::Behavior)
                .order(20)
                .hint(QStringLiteral("Seed number consumed by the start component when simulation begins."))
                .build(),
        });

    cme::helper::property_schemas::addTarget(&bundle, "component/stop",
                                             {
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .required()
                                                     .defaultValue(QString())
                                                     .textField()
                                                     .section(SchemaFieldSection::Identity)
                                                     .order(1)
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

QString StartStopPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList StartStopPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList StartStopPropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}
