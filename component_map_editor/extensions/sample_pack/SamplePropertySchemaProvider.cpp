#include "SamplePropertySchemaProvider.h"

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "utils/PropertySchemaHelper.h"

namespace
{

using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.propertySchema");
    bundle.set_schema_version("1.0.0");

    cme::helper::property_schemas::addTarget(
        &bundle, "component/start",
        {
            cme::helper::property_schemas::Field("title")
                .type(SchemaFieldType::String)
                .title("Title")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(1)
                .hint("Human-friendly label shown on the graph.")
                .build(),
            cme::helper::property_schemas::Field("inputNumber")
                .type(SchemaFieldType::Number)
                .title("Input Number")
                .required()
                .defaultValue(0)
                .spinBox(-1000000, 1000000)
                .section(SchemaFieldSection::Behavior)
                .order(20)
                .hint("Seed number consumed by the start component when simulation begins.")
                .build(),
            cme::helper::property_schemas::Field("icon")
                .type(SchemaFieldType::String)
                .title("Icon")
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Appearance)
                .order(30)
                .hint("FontAwesome icon key, for example 'play' or 'stop'.")
                .build(),
            cme::helper::property_schemas::Field("color")
                .type(SchemaFieldType::String)
                .title("Color")
                .required()
                .defaultValue(QStringLiteral("#66bb6a"))
                .textField()
                .section(SchemaFieldSection::Appearance)
                .order(31)
                .build(),
            cme::helper::property_schemas::Field("shape")
                .type(SchemaFieldType::Enum)
                .title("Shape")
                .required()
                .defaultValue(QStringLiteral("rounded"))
                .dropdown(cme::runtime::Options::fixed({QStringLiteral("rounded"), QStringLiteral("rectangle")}))
                .section(SchemaFieldSection::Appearance)
                .order(32)
                .build(),
        });

    cme::helper::property_schemas::addTarget(&bundle, "component/process",
                                             {
                                                 cme::helper::property_schemas::Field("title")
                                                     .type(SchemaFieldType::String)
                                                     .title("Title")
                                                     .required()
                                                     .defaultValue(QString())
                                                     .textField()
                                                     .section(SchemaFieldSection::Identity)
                                                     .order(1)
                                                     .hint("Display title used in the component body.")
                                                     .build(),
                                                 cme::helper::property_schemas::Field("description")
                                                     .type(SchemaFieldType::String)
                                                     .title("Description")
                                                     .defaultValue(QString())
                                                     .textArea()
                                                     .section(SchemaFieldSection::Behavior)
                                                     .order(20)
                                                     .hint("Optional implementation notes for this process.")
                                                     .build(),
                                                 cme::helper::property_schemas::Field("addValue")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Add Value")
                                                     .required()
                                                     .defaultValue(9)
                                                     .spinBox(-1000000, 1000000)
                                                     .section(SchemaFieldSection::Behavior)
                                                     .order(21)
                                                     .hint("Number added to the current simulation value (default +9).")
                                                     .build(),
                                                 cme::helper::property_schemas::Field("x")
                                                     .type(SchemaFieldType::Number)
                                                     .title("X")
                                                     .required()
                                                     .defaultValue(0)
                                                     .spinBox(-50000, 50000)
                                                     .section(SchemaFieldSection::Layout)
                                                     .order(40)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("y")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Y")
                                                     .required()
                                                     .defaultValue(0)
                                                     .spinBox(-50000, 50000)
                                                     .section(SchemaFieldSection::Layout)
                                                     .order(41)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("width")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Width")
                                                     .required()
                                                     .defaultValue(160)
                                                     .spinBox(40, 1200)
                                                     .section(SchemaFieldSection::Layout)
                                                     .order(42)
                                                     .build(),
                                                 cme::helper::property_schemas::Field("height")
                                                     .type(SchemaFieldType::Number)
                                                     .title("Height")
                                                     .required()
                                                     .defaultValue(96)
                                                     .spinBox(40, 1200)
                                                     .section(SchemaFieldSection::Layout)
                                                     .order(43)
                                                     .build(),
                                             });

    cme::helper::property_schemas::addTarget(
        &bundle, "component/stop",
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
            cme::helper::property_schemas::Field("description")
                .type(SchemaFieldType::String)
                .title("Description")
                .defaultValue(QString())
                .textArea()
                .section(SchemaFieldSection::Behavior)
                .order(20)
                .hint("Optional notes shown in the inspector.")
                .build(),
            cme::helper::property_schemas::Field("icon")
                .type(SchemaFieldType::String)
                .title("Icon")
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Appearance)
                .order(30)
                .hint("FontAwesome icon key, for example 'stop' or 'flag-checkered'.")
                .build(),
            cme::helper::property_schemas::Field("color")
                .type(SchemaFieldType::String)
                .title("Color")
                .required()
                .defaultValue(QStringLiteral("#ef5350"))
                .textField()
                .section(SchemaFieldSection::Appearance)
                .order(31)
                .build(),
        });

    const QVariantList sideOptions{
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Auto")}, {QStringLiteral("value"), -1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Top")}, {QStringLiteral("value"), 0}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Right")}, {QStringLiteral("value"), 1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Bottom")}, {QStringLiteral("value"), 2}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Left")}, {QStringLiteral("value"), 3}}};
    cme::helper::property_schemas::addTarget(
        &bundle, "connection/flow",
        {
            cme::helper::property_schemas::Field("label")
                .type(SchemaFieldType::String)
                .title("Label")
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(0)
                .build(),
            cme::helper::property_schemas::Field("id")
                .type(SchemaFieldType::String)
                .title("Connection ID")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(1)
                .build(),
            cme::helper::property_schemas::Field("sourceId")
                .type(SchemaFieldType::String)
                .title("Source Component ID")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(2)
                .build(),
            cme::helper::property_schemas::Field("targetId")
                .type(SchemaFieldType::String)
                .title("Target Component ID")
                .required()
                .defaultValue(QString())
                .textField()
                .section(SchemaFieldSection::Identity)
                .order(3)
                .build(),
            cme::helper::property_schemas::Field("sourceSide")
                .type(SchemaFieldType::Enum)
                .title("Source Side")
                .required()
                .defaultValue(-1)
                .dropdown(cme::runtime::Options::fixed(sideOptions))
                .section(SchemaFieldSection::Routing)
                .order(20)
                .build(),
            cme::helper::property_schemas::Field("targetSide")
                .type(SchemaFieldType::Enum)
                .title("Target Side")
                .required()
                .defaultValue(-1)
                .dropdown(cme::runtime::Options::fixed(sideOptions))
                .section(SchemaFieldSection::Routing)
                .order(21)
                .build(),
            cme::helper::property_schemas::Field("tokenKey")
                .type(SchemaFieldType::String)
                .title("Token Key")
                .defaultValue(QString())
                .dropdown(cme::runtime::Options::tokenKeys())
                .section(SchemaFieldSection::Routing)
                .order(22)
                .hint("Routing token key used for connection payload selection. Token options are sourced from "
                      "connection token keys, execution-state keys, and schema key defaults.")
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

QString SamplePropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList SamplePropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList SamplePropertySchemaProvider::propertySchema(const QString& targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(schemaBundle(), targetId);
}
