#include "customizepropertyschemaprovider.h"

#include <initializer_list>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include "extensions/runtime/templates/PropertySchemaTemplateAdapter.h"
#include "provider_templates.pb.h"

namespace {

google::protobuf::Value toProtoValue(const QJsonValue &value)
{
    google::protobuf::Value out;

    if (value.isNull() || value.isUndefined()) {
        out.set_null_value(google::protobuf::NULL_VALUE);
        return out;
    }

    if (value.isBool()) {
        out.set_bool_value(value.toBool());
        return out;
    }

    if (value.isDouble()) {
        out.set_number_value(value.toDouble());
        return out;
    }

    if (value.isString()) {
        out.set_string_value(value.toString().toStdString());
        return out;
    }

    if (value.isArray()) {
        google::protobuf::ListValue *list = out.mutable_list_value();
        const QJsonArray array = value.toArray();
        for (const QJsonValue &item : array)
            *list->add_values() = toProtoValue(item);
        return out;
    }

    const QJsonObject object = value.toObject();
    google::protobuf::Struct *st = out.mutable_struct_value();
    for (auto it = object.begin(); it != object.end(); ++it)
        (*st->mutable_fields())[it.key().toStdString()] = toProtoValue(it.value());
    return out;
}

google::protobuf::Value toProtoValue(const QVariant &value)
{
    return toProtoValue(QJsonValue::fromVariant(value));
}

cme::templates::v1::PropertySchemaFieldTemplate makeField(
    const char *key,
    const char *type,
    const char *title,
    bool required,
    const QVariant &defaultValue,
    const char *editor,
    const char *section,
    int order,
    const QString &hint = QString(),
    const QVariantMap &validation = {},
    const QVariantMap &visibleWhen = {},
    const QVariantList &options = {})
{
    cme::templates::v1::PropertySchemaFieldTemplate field;
    field.set_key(key);
    field.set_type(type);
    field.set_title(title);
    field.set_required(required);
    field.set_editor(editor);
    field.set_section(section);
    field.set_order(order);
    if (!hint.isEmpty())
        field.set_hint(hint.toStdString());

    *field.mutable_default_value() = toProtoValue(defaultValue);

    for (const QVariant &option : options)
        *field.add_options() = toProtoValue(option);

    for (auto it = validation.constBegin(); it != validation.constEnd(); ++it)
        (*field.mutable_validation())[it.key().toStdString()] = toProtoValue(it.value());

    for (auto it = visibleWhen.constBegin(); it != visibleWhen.constEnd(); ++it)
        (*field.mutable_visible_when())[it.key().toStdString()] = toProtoValue(it.value());

    return field;
}

void addTarget(cme::templates::v1::PropertySchemaTemplateBundle *bundle,
               const char *targetId,
               const std::initializer_list<cme::templates::v1::PropertySchemaFieldTemplate> &fields)
{
    cme::templates::v1::PropertySchemaTargetTemplate *target = bundle->add_targets();
    target->set_target_id(targetId);
    for (const auto &field : fields)
        *target->add_entries() = field;
}

cme::templates::v1::PropertySchemaTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema");
    bundle.set_schema_version("1.0.0");

    addTarget(&bundle,
              "component/start",
              {
                  makeField("title", "string", "Title", true, QString(), "textfield", "Identity", 1,
                            QStringLiteral("Human-friendly label shown on the graph.")),
                  makeField("inputNumber", "number", "Input Number", true, 0, "spinbox", "Behavior", 20,
                            QStringLiteral("Seed number consumed by the start component when simulation begins."),
                            QVariantMap{{QStringLiteral("min"), -1000000}, {QStringLiteral("max"), 1000000}}),
                  makeField("icon", "string", "Icon", false, QString(), "textfield", "Appearance", 30,
                            QStringLiteral("FontAwesome icon key, for example 'play' or 'stop'.")),
                  makeField("color", "string", "Color", true, QStringLiteral("#66bb6a"), "textfield", "Appearance", 31),
                  makeField("shape", "enum", "Shape", true, QStringLiteral("rounded"), "dropdown", "Appearance", 32,
                            QString(), {}, {},
                            QVariantList{ QStringLiteral("rounded"), QStringLiteral("rectangle") })
              });

    addTarget(&bundle,
              "component/condition",
              {
                  makeField("title", "string", "Title", true, QString(), "textfield", "Identity", 1,
                            QStringLiteral("Human-friendly label shown on the graph.")),
                  makeField("description", "string", "Description", false, QString(), "textarea", "Behavior", 20,
                            QStringLiteral("Optional implementation notes for this condition.")),
                  makeField("icon", "string", "Icon", false, QString(), "textfield", "Appearance", 30,
                            QStringLiteral("FontAwesome icon key, for example 'question' or 'exclamation'.")),
                  makeField("color", "string", "Color", true, QStringLiteral("#ffca28"), "textfield", "Appearance", 31)
              });

    addTarget(&bundle,
              "component/process",
              {
                  makeField("title", "string", "Title", true, QString(), "textfield", "Identity", 1,
                            QStringLiteral("Display title used in the component body.")),
                  makeField("description", "string", "Description", false, QString(), "textarea", "Behavior", 20,
                            QStringLiteral("Optional implementation notes for this process.")),
                  makeField("addValue", "number", "Add Value", true, 9, "spinbox", "Behavior", 21,
                            QStringLiteral("Number added to the current simulation value (default +9)."),
                            QVariantMap{{QStringLiteral("min"), -1000000}, {QStringLiteral("max"), 1000000}}),
                  makeField("x", "number", "X", true, 0, "spinbox", "Layout", 40,
                            QString(),
                            QVariantMap{{QStringLiteral("min"), -50000}, {QStringLiteral("max"), 50000}}),
                  makeField("y", "number", "Y", true, 0, "spinbox", "Layout", 41,
                            QString(),
                            QVariantMap{{QStringLiteral("min"), -50000}, {QStringLiteral("max"), 50000}}),
                  makeField("width", "number", "Width", true, 160, "spinbox", "Layout", 42,
                            QString(),
                            QVariantMap{{QStringLiteral("min"), 40}, {QStringLiteral("max"), 1200}}),
                  makeField("height", "number", "Height", true, 96, "spinbox", "Layout", 43,
                            QString(),
                            QVariantMap{{QStringLiteral("min"), 40}, {QStringLiteral("max"), 1200}})
              });

    addTarget(&bundle,
              "component/stop",
              {
                  makeField("title", "string", "Title", true, QString(), "textfield", "Identity", 1),
                  makeField("description", "string", "Description", false, QString(), "textarea", "Behavior", 20,
                            QStringLiteral("Optional notes shown in the inspector.")),
                  makeField("icon", "string", "Icon", false, QString(), "textfield", "Appearance", 30,
                            QStringLiteral("FontAwesome icon key, for example 'stop' or 'flag-checkered'.")),
                  makeField("color", "string", "Color", true, QStringLiteral("#ef5350"), "textfield", "Appearance", 31)
              });

    const QVariantList sideOptions{
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Auto")},   {QStringLiteral("value"), -1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Top")},    {QStringLiteral("value"),  0}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Right")},  {QStringLiteral("value"),  1}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Bottom")}, {QStringLiteral("value"),  2}},
        QVariantMap{{QStringLiteral("text"), QStringLiteral("Left")},   {QStringLiteral("value"),  3}}
    };

    addTarget(&bundle,
              "connection/flow",
              {
                  makeField("label", "string", "Label", false, QString(), "textfield", "Identity", 0),
                  makeField("id", "string", "Connection ID", true, QString(), "textfield", "Identity", 1),
                  makeField("sourceId", "string", "Source Component ID", true, QString(), "textfield", "Identity", 2),
                  makeField("targetId", "string", "Target Component ID", true, QString(), "textfield", "Identity", 3),
                  makeField("sourceSide", "enum", "Source Side", true, -1, "dropdown", "Routing", 20,
                            QString(), {}, {}, sideOptions),
                  makeField("targetSide", "enum", "Target Side", true, -1, "dropdown", "Routing", 21,
                            QString(), {}, {}, sideOptions)
              });

    return bundle;
}

const cme::templates::v1::PropertySchemaTemplateBundle &schemaBundle()
{
    static const cme::templates::v1::PropertySchemaTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString CustomizePropertySchemaProvider::providerId() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::providerId(schemaBundle());
}

QStringList CustomizePropertySchemaProvider::schemaTargets() const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaTargets(schemaBundle());
}

QVariantList CustomizePropertySchemaProvider::propertySchema(const QString &targetId) const
{
    return cme::runtime::templates::PropertySchemaTemplateAdapter::schemaForTarget(
        schemaBundle(), targetId);
}