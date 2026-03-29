#include "ComponentTypeTemplateAdapter.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace {

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

QString ComponentTypeTemplateAdapter::providerId(const cme::templates::v1::ComponentTypeTemplateBundle &bundle)
{
    return QString::fromStdString(bundle.provider_id());
}

QStringList ComponentTypeTemplateAdapter::componentTypeIds(const cme::templates::v1::ComponentTypeTemplateBundle &bundle)
{
    QStringList out;
    out.reserve(bundle.component_types_size());
    for (const cme::templates::v1::ComponentTypeTemplate &type : bundle.component_types()) {
        const QString id = QString::fromStdString(type.id()).trimmed();
        if (!id.isEmpty())
            out.append(id);
    }
    return out;
}

QVariantMap ComponentTypeTemplateAdapter::componentTypeDescriptor(
    const cme::templates::v1::ComponentTypeTemplateBundle &bundle,
    const QString &componentTypeId)
{
    const std::string componentTypeIdStd = componentTypeId.toStdString();
    for (const cme::templates::v1::ComponentTypeTemplate &type : bundle.component_types()) {
        if (type.id() != componentTypeIdStd)
            continue;

        QVariantMap descriptor{
            { QStringLiteral("id"), QString::fromStdString(type.id()) },
            { QStringLiteral("title"), QString::fromStdString(type.title()) },
            { QStringLiteral("category"), QString::fromStdString(type.category()) },
            { QStringLiteral("defaultWidth"), type.default_width() },
            { QStringLiteral("defaultHeight"), type.default_height() },
            { QStringLiteral("defaultColor"), QString::fromStdString(type.default_color()) },
            { QStringLiteral("allowIncoming"), type.allow_incoming() },
            { QStringLiteral("allowOutgoing"), type.allow_outgoing() }
        };

        for (const auto &kv : type.extra()) {
            const QString key = QString::fromStdString(kv.first);
            if (!descriptor.contains(key))
                descriptor.insert(key, valueToVariant(kv.second));
        }
        return descriptor;
    }

    return {};
}

QVariantMap ComponentTypeTemplateAdapter::defaultComponentProperties(
    const cme::templates::v1::ComponentTypeTemplateBundle &bundle,
    const QString &componentTypeId)
{
    const std::string componentTypeIdStd = componentTypeId.toStdString();
    for (const cme::templates::v1::ComponentTypeDefaultPropertiesTemplate &defaults : bundle.defaults()) {
        if (defaults.component_type_id() != componentTypeIdStd)
            continue;

        QVariantMap properties;
        for (const auto &kv : defaults.properties())
            properties.insert(QString::fromStdString(kv.first), valueToVariant(kv.second));
        return properties;
    }

    return {};
}

QVariant ComponentTypeTemplateAdapter::valueToVariant(const google::protobuf::Value &value)
{
    return protoToQJsonValue(value).toVariant();
}

} // namespace cme::runtime::templates
