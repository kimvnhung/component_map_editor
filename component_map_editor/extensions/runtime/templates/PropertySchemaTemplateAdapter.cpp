#include "PropertySchemaTemplateAdapter.h"

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

QString PropertySchemaTemplateAdapter::providerId(const cme::templates::v1::PropertySchemaTemplateBundle &bundle)
{
    return QString::fromStdString(bundle.provider_id());
}

QStringList PropertySchemaTemplateAdapter::schemaTargets(const cme::templates::v1::PropertySchemaTemplateBundle &bundle)
{
    QStringList targets;
    targets.reserve(bundle.targets_size());
    for (const cme::templates::v1::PropertySchemaTargetTemplate &target : bundle.targets()) {
        const QString targetId = QString::fromStdString(target.target_id()).trimmed();
        if (!targetId.isEmpty())
            targets.append(targetId);
    }
    return targets;
}

QVariantList PropertySchemaTemplateAdapter::schemaForTarget(
    const cme::templates::v1::PropertySchemaTemplateBundle &bundle,
    const QString &targetId)
{
    const std::string targetIdStd = targetId.toStdString();
    for (const cme::templates::v1::PropertySchemaTargetTemplate &target : bundle.targets()) {
        if (target.target_id() != targetIdStd)
            continue;

        QVariantList rows;
        rows.reserve(target.entries_size());
        for (const cme::templates::v1::PropertySchemaFieldTemplate &field : target.entries())
            rows.append(fieldToVariantMap(field));
        return rows;
    }

    return {};
}

QVariant PropertySchemaTemplateAdapter::valueToVariant(const google::protobuf::Value &value)
{
    return protoToQJsonValue(value).toVariant();
}

QVariantMap PropertySchemaTemplateAdapter::fieldToVariantMap(
    const cme::templates::v1::PropertySchemaFieldTemplate &field)
{
    QVariantMap row;
    row.insert(QStringLiteral("key"), QString::fromStdString(field.key()));
    row.insert(QStringLiteral("type"), QString::fromStdString(field.type()));
    row.insert(QStringLiteral("title"), QString::fromStdString(field.title()));
    row.insert(QStringLiteral("required"), field.required());
    row.insert(QStringLiteral("editor"), QString::fromStdString(field.editor()));
    row.insert(QStringLiteral("section"), QString::fromStdString(field.section()));
    row.insert(QStringLiteral("order"), field.order());
    row.insert(QStringLiteral("hint"), QString::fromStdString(field.hint()));

    if (field.default_value().kind_case() != google::protobuf::Value::KIND_NOT_SET)
        row.insert(QStringLiteral("defaultValue"), valueToVariant(field.default_value()));

    QVariantList options;
    options.reserve(field.options_size());
    for (const google::protobuf::Value &option : field.options())
        options.append(valueToVariant(option));
    if (!options.isEmpty())
        row.insert(QStringLiteral("options"), options);

    QVariantMap validation;
    for (const auto &kv : field.validation())
        validation.insert(QString::fromStdString(kv.first), valueToVariant(kv.second));
    if (!validation.isEmpty())
        row.insert(QStringLiteral("validation"), validation);

    QVariantMap visibleWhen;
    for (const auto &kv : field.visible_when())
        visibleWhen.insert(QString::fromStdString(kv.first), valueToVariant(kv.second));
    if (!visibleWhen.isEmpty())
        row.insert(QStringLiteral("visibleWhen"), visibleWhen);

    for (const auto &kv : field.extra()) {
        const QString key = QString::fromStdString(kv.first);
        if (!row.contains(key))
            row.insert(key, valueToVariant(kv.second));
    }

    return row;
}

} // namespace cme::runtime::templates
