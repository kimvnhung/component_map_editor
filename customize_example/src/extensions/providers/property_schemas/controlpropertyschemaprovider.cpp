#include "controlpropertyschemaprovider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include <utils/PropertySchemaHelper.h>

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;
using cme::runtime::SchemaOptionsSource;

const QString kTokenKeyHint = QStringLiteral(
    "Token options are sourced from connection token keys, execution-state keys, and schema key defaults.");

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema.control");
    bundle.set_schema_version("1.0.0");

    cme::helper::property_schemas::addTarget(
        &bundle, "component/control/loop",
        {
            // cme::helper::property_schemas::makeField("iterKey", SchemaFieldType::String, "Iter Key", true,
            // QStringLiteral("iter"), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 1, kTokenKeyHint, {},
            // {}, {}, SchemaOptionsSource::TokenKeys), cme::helper::property_schemas::makeField("maxIterKey",
            // SchemaFieldType::String, "Max Iter Key", true, QStringLiteral("maxIter"), SchemaFieldWidget::Dropdown,
            // SchemaFieldSection::Context, 2, kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
            // cme::helper::property_schemas::makeField("continueKey", SchemaFieldType::String, "Continue Key", true,
            // QStringLiteral("continueLoop"), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 3,
            // kTokenKeyHint, {}, {}, {}, SchemaOptionsSource::TokenKeys),
            // cme::helper::property_schemas::makeField("conditionKey", SchemaFieldType::String, "Condition Key", true,
            // QStringLiteral("condition"), SchemaFieldWidget::Dropdown, SchemaFieldSection::Context, 4, kTokenKeyHint,
            // {}, {}, {}, SchemaOptionsSource::TokenKeys), cme::helper::property_schemas::makeField("iter",
            // SchemaFieldType::Number, "Fallback Iter", false, 0, SchemaFieldWidget::SpinBox,
            // SchemaFieldSection::Fallback, 20), cme::helper::property_schemas::makeField("maxIter",
            // SchemaFieldType::Number, "Fallback Max Iter", false, 10, SchemaFieldWidget::SpinBox,
            // SchemaFieldSection::Fallback, 21), cme::helper::property_schemas::makeField("condition",
            // SchemaFieldType::Boolean, "Fallback Condition", false, true, SchemaFieldWidget::Checkbox,
            // SchemaFieldSection::Fallback, 22)
            cme::helper::property_schemas::Field("iterKey")
                .type(SchemaFieldType::String)
                .title("Iter Key")
                .required()
                .defaultValue(QStringLiteral("iter"))
                .dropdown(Options::tokenKeys())
                .section(SchemaFieldSection::Context)
                .order(1)
                .hint(kTokenKeyHint)
                .build(),
            cme::helper::property_schemas::Field("maxIterKey")
                .type(SchemaFieldType::String)
                .title("Max Iter Key")
                .required()
                .defaultValue(QStringLiteral("maxIter"))
                .dropdown(Options::tokenKeys())
                .section(SchemaFieldSection::Context)
                .order(2)
                .hint(kTokenKeyHint)
                .build(),
            cme::helper::property_schemas::Field("continueKey")
                .type(SchemaFieldType::String)
                .title("Continue Key")
                .required()
                .defaultValue(QStringLiteral("continueLoop"))
                .dropdown(Options::tokenKeys())
                .section(SchemaFieldSection::Context)
                .order(3)
                .hint(kTokenKeyHint)
                .build(),
            cme::helper::property_schemas::Field("conditionKey")
                .type(SchemaFieldType::String)
                .title("Condition Key")
                .required()
                .defaultValue(QStringLiteral("condition"))
                .dropdown(Options::tokenKeys())
                .section(SchemaFieldSection::Context)
                .order(4)
                .hint(kTokenKeyHint)
                .build(),
            cme::helper::property_schemas::Field("iter")
                .type(SchemaFieldType::Number)
                .title("Fallback Iter")
                .defaultValue(0)
                .spinBox()
                .section(SchemaFieldSection::Fallback)
                .order(20)
                .build(),
            cme::helper::property_schemas::Field("maxIter")
                .type(SchemaFieldType::Number)
                .title("Fallback Max Iter")
                .defaultValue(10)
                .spinBox()
                .section(SchemaFieldSection::Fallback)
                .order(21)
                .build(),
            cme::helper::property_schemas::Field("condition")
                .type(SchemaFieldType::Boolean)
                .title("Fallback Condition")
                .defaultValue(true)
                .checkbox()
                .section(SchemaFieldSection::Fallback)
                .order(22)
                .build(),
        });

    cme::helper::property_schemas::addTarget(&bundle, "component/control/ifelse",
                                             {
                                                 cme::helper::property_schemas::Field("conditionKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("Condition Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("condition"))
                                                     .dropdown(Options::tokenKeys())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(1)
                                                     .hint(kTokenKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("trueRouteKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("True Route Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("routeTrue"))
                                                     .dropdown(Options::tokenKeys())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(2)
                                                     .hint(kTokenKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("falseRouteKey")
                                                     .type(SchemaFieldType::String)
                                                     .title("False Route Key")
                                                     .required()
                                                     .defaultValue(QStringLiteral("routeFalse"))
                                                     .dropdown(Options::tokenKeys())
                                                     .section(SchemaFieldSection::Context)
                                                     .order(3)
                                                     .hint(kTokenKeyHint)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("condition")
                                                     .type(SchemaFieldType::Boolean)
                                                     .title("Fallback Condition")
                                                     .defaultValue(false)
                                                     .checkbox()
                                                     .section(SchemaFieldSection::Fallback)
                                                     .order(20)
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

QString ControlPropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList ControlPropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList ControlPropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}
