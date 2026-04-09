#include "TemplateProtoHelpers.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace {

google::protobuf::Value qJsonToProtoValue(const QJsonValue &value)
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
            *list->add_values() = qJsonToProtoValue(item);
        return out;
    }

    const QJsonObject object = value.toObject();
    google::protobuf::Struct *st = out.mutable_struct_value();
    for (auto it = object.begin(); it != object.end(); ++it)
        (*st->mutable_fields())[it.key().toStdString()] = qJsonToProtoValue(it.value());
    return out;
}

QJsonValue protoToQJsonValue(const google::protobuf::Value &value)
{
    switch (value.kind_case()) {
    case google::protobuf::Value::kNullValue:
        return QJsonValue();
    case google::protobuf::Value::kBoolValue:
        return QJsonValue(value.bool_value());
    case google::protobuf::Value::kNumberValue:
        return QJsonValue(value.number_value());
    case google::protobuf::Value::kStringValue:
        return QJsonValue(QString::fromStdString(value.string_value()));
    case google::protobuf::Value::kStructValue: {
        QJsonObject object;
        for (const auto &kv : value.struct_value().fields())
            object.insert(QString::fromStdString(kv.first), protoToQJsonValue(kv.second));
        return object;
    }
    case google::protobuf::Value::kListValue: {
        QJsonArray array;
        for (const google::protobuf::Value &item : value.list_value().values())
            array.append(protoToQJsonValue(item));
        return array;
    }
    case google::protobuf::Value::KIND_NOT_SET:
    default:
        return QJsonValue();
    }
}

} // namespace

namespace cme::runtime::templates {

google::protobuf::Value variantToProtoValue(const QVariant &value)
{
    return qJsonToProtoValue(QJsonValue::fromVariant(value));
}

QVariant protoValueToVariant(const google::protobuf::Value &value)
{
    return protoToQJsonValue(value).toVariant();
}

cme::templates::v1::ComponentTypeTemplate makeComponentTypeTemplate(
    const QString &id,
    const QString &title,
    const QString &category,
    double defaultWidth,
    double defaultHeight,
    const QString &defaultColor,
    bool allowIncoming,
    bool allowOutgoing)
{
    cme::templates::v1::ComponentTypeTemplate type;
    type.set_id(id.toStdString());
    type.set_title(title.toStdString());
    type.set_category(category.toStdString());
    type.set_default_width(defaultWidth);
    type.set_default_height(defaultHeight);
    type.set_default_color(defaultColor.toStdString());
    type.set_allow_incoming(allowIncoming);
    type.set_allow_outgoing(allowOutgoing);
    return type;
}

cme::templates::v1::ComponentTypeDefaultPropertiesTemplate makeComponentTypeDefaultsTemplate(
    const QString &componentTypeId,
    const QVariantMap &properties)
{
    cme::templates::v1::ComponentTypeDefaultPropertiesTemplate defaults;
    defaults.set_component_type_id(componentTypeId.toStdString());
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it)
        (*defaults.mutable_properties())[it.key().toStdString()] = variantToProtoValue(it.value());
    return defaults;
}

cme::templates::v1::ConnectionPolicyRuleTemplate makeConnectionPolicyRuleTemplate(
    const QString &sourceTypeId,
    const QString &targetTypeId,
    bool allowed,
    const QString &reason)
{
    cme::templates::v1::ConnectionPolicyRuleTemplate rule;
    rule.set_source_type_id(sourceTypeId.toStdString());
    rule.set_target_type_id(targetTypeId.toStdString());
    rule.set_allowed(allowed);
    rule.set_reason(reason.toStdString());
    return rule;
}

cme::templates::v1::ConnectionTransportMetadataTemplate makeConnectionTransportMetadataTemplate(
    const QString &payloadSchemaId,
    const QString &payloadType,
    const QString &transportMode,
    const QString &mergeHint)
{
    cme::templates::v1::ConnectionTransportMetadataTemplate transport;
    transport.set_payload_schema_id(payloadSchemaId.toStdString());
    transport.set_payload_type(payloadType.toStdString());
    transport.set_transport_mode(transportMode.toStdString());
    transport.set_merge_hint(mergeHint.toStdString());
    return transport;
}

QString formatUnknownConnectionReason(const cme::ConnectionPolicyContext &context,
                                     const QString &pattern)
{
    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());
    return pattern.arg(sourceTypeId, targetTypeId);
}

} // namespace cme::runtime::templates
