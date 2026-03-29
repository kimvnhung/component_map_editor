#include "SampleComponentTypeProvider.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include "extensions/runtime/templates/ComponentTypeTemplateAdapter.h"
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

cme::templates::v1::ComponentTypeTemplate makeType(
    const char *id,
    const char *title,
    const char *category,
    double defaultWidth,
    double defaultHeight,
    const char *defaultColor,
    bool allowIncoming,
    bool allowOutgoing)
{
    cme::templates::v1::ComponentTypeTemplate type;
    type.set_id(id);
    type.set_title(title);
    type.set_category(category);
    type.set_default_width(defaultWidth);
    type.set_default_height(defaultHeight);
    type.set_default_color(defaultColor);
    type.set_allow_incoming(allowIncoming);
    type.set_allow_outgoing(allowOutgoing);
    return type;
}

cme::templates::v1::ComponentTypeDefaultPropertiesTemplate makeDefaults(
    const char *componentTypeId,
    const QVariantMap &properties)
{
    cme::templates::v1::ComponentTypeDefaultPropertiesTemplate defaults;
    defaults.set_component_type_id(componentTypeId);
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it)
        (*defaults.mutable_properties())[it.key().toStdString()] = toProtoValue(it.value());
    return defaults;
}

cme::templates::v1::ComponentTypeTemplateBundle buildTemplateBundle()
{
    cme::templates::v1::ComponentTypeTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.componentTypes");
    bundle.set_schema_version("1.0.0");

    *bundle.add_component_types() = makeType(
        SampleComponentTypeProvider::TypeStart, "Start", "control", 92.0, 92.0, "#66bb6a", false, true);
    *bundle.add_component_types() = makeType(
        SampleComponentTypeProvider::TypeProcess, "Process", "work", 164.0, 100.0, "#4fc3f7", true, true);
    *bundle.add_component_types() = makeType(
        SampleComponentTypeProvider::TypeStop, "Stop", "control", 92.0, 92.0, "#ef5350", true, false);

    *bundle.add_defaults() = makeDefaults(SampleComponentTypeProvider::TypeStart,
                                          QVariantMap{{QStringLiteral("inputNumber"), 0}});
    *bundle.add_defaults() = makeDefaults(
        SampleComponentTypeProvider::TypeProcess,
        QVariantMap{{QStringLiteral("addValue"), 9}, {QStringLiteral("description"), QString()}});

    return bundle;
}

const cme::templates::v1::ComponentTypeTemplateBundle &templateBundle()
{
    static const cme::templates::v1::ComponentTypeTemplateBundle kBundle = buildTemplateBundle();
    return kBundle;
}

} // namespace

QString SampleComponentTypeProvider::providerId() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::providerId(templateBundle());
}

QStringList SampleComponentTypeProvider::componentTypeIds() const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeIds(templateBundle());
}

QVariantMap SampleComponentTypeProvider::componentTypeDescriptor(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::componentTypeDescriptor(
        templateBundle(), componentTypeId);
}

QVariantMap SampleComponentTypeProvider::defaultComponentProperties(const QString &componentTypeId) const
{
    return cme::runtime::templates::ComponentTypeTemplateAdapter::defaultComponentProperties(
        templateBundle(), componentTypeId);
}