// component_map_editor/adapters/PolicyAdapter.cpp

#include "PolicyAdapter.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToConnectionPolicyContext(
    const QVariantMap &variant,
    cme::ConnectionPolicyContext &proto_out)
{
    if (variant.contains("sourceTypeId"))
        proto_out.set_source_type_id(variant["sourceTypeId"].toString().toStdString());

    if (variant.contains("targetTypeId"))
        proto_out.set_target_type_id(variant["targetTypeId"].toString().toStdString());

    if (variant.contains("sourcePort"))
        proto_out.set_source_port(variant["sourcePort"].toString().toStdString());

    if (variant.contains("targetPort"))
        proto_out.set_target_port(variant["targetPort"].toString().toStdString());

    if (variant.contains("extra")) {
        auto extra = variant["extra"].toMap();
        for (auto it = extra.begin(); it != extra.end(); ++it) {
            (*proto_out.mutable_extra())[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    return ConversionError();
}

ConversionError variantMapToComponentTypeDefaults(
    const QVariantMap &variant,
    cme::ComponentTypeDefaults &proto_out)
{
    if (!variant.contains("typeId"))
        return ConversionError("Missing required field: typeId");

    proto_out.set_type_id(variant["typeId"].toString().toStdString());

    bool ok_x = false, ok_y = false;
    double default_x = variant.value("defaultX", 0.0).toDouble(&ok_x);
    double default_y = variant.value("defaultY", 0.0).toDouble(&ok_y);

    if (!ok_x && variant.contains("defaultX"))
        return ConversionError("Field 'defaultX' is not a valid double");
    if (!ok_y && variant.contains("defaultY"))
        return ConversionError("Field 'defaultY' is not a valid double");

    proto_out.set_default_x(default_x);
    proto_out.set_default_y(default_y);

    if (variant.contains("defaultProperties")) {
        auto props = variant["defaultProperties"].toMap();
        for (auto it = props.begin(); it != props.end(); ++it) {
            (*proto_out.mutable_default_properties())[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    return ConversionError();
}

ConversionError variantMapToConnectionProperties(
    const QVariantMap &variant,
    cme::ConnectionProperties &proto_out)
{
    if (!variant.contains("connectionId"))
        return ConversionError("Missing required field: connectionId");

    proto_out.set_connection_id(variant["connectionId"].toString().toStdString());

    if (variant.contains("properties")) {
        auto props = variant["properties"].toMap();
        for (auto it = props.begin(); it != props.end(); ++it) {
            (*proto_out.mutable_properties())[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    return ConversionError();
}

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap connectionPolicyContextToVariantMap(
    const cme::ConnectionPolicyContext &proto)
{
    QVariantMap result;

    if (!proto.source_type_id().empty())
        result["sourceTypeId"] = QString::fromStdString(proto.source_type_id());

    if (!proto.target_type_id().empty())
        result["targetTypeId"] = QString::fromStdString(proto.target_type_id());

    if (!proto.source_port().empty())
        result["sourcePort"] = QString::fromStdString(proto.source_port());

    if (!proto.target_port().empty())
        result["targetPort"] = QString::fromStdString(proto.target_port());

    QVariantMap extra;
    for (const auto &kv : proto.extra())
        extra[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    if (!extra.isEmpty())
        result["extra"] = extra;

    return result;
}

QVariantMap componentTypeDefaultsToVariantMap(
    const cme::ComponentTypeDefaults &proto)
{
    QVariantMap result;
    result["typeId"] = QString::fromStdString(proto.type_id());
    result["defaultX"] = proto.default_x();
    result["defaultY"] = proto.default_y();

    QVariantMap props;
    for (const auto &kv : proto.default_properties())
        props[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    if (!props.isEmpty())
        result["defaultProperties"] = props;

    return result;
}

QVariantMap connectionPropertiesToVariantMap(
    const cme::ConnectionProperties &proto)
{
    QVariantMap result;
    result["connectionId"] = QString::fromStdString(proto.connection_id());

    QVariantMap props;
    for (const auto &kv : proto.properties())
        props[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    if (!props.isEmpty())
        result["properties"] = props;

    return result;
}

} // namespace cme::adapter
