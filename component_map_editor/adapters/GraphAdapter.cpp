// component_map_editor/adapters/GraphAdapter.cpp

#include "GraphAdapter.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToComponentData(
    const QVariantMap &variant,
    cme::ComponentData &proto_out)
{
    if (!variant.contains("id"))
        return ConversionError("Missing required field: id");
    if (!variant.contains("typeId"))
        return ConversionError("Missing required field: typeId");

    proto_out.set_id(variant["id"].toString().toStdString());
    proto_out.set_type_id(variant["typeId"].toString().toStdString());

    bool ok_x = false, ok_y = false;
    double x = variant.value("x", 0.0).toDouble(&ok_x);
    double y = variant.value("y", 0.0).toDouble(&ok_y);

    if (!ok_x && variant.contains("x"))
        return ConversionError("Field 'x' is not a valid double");
    if (!ok_y && variant.contains("y"))
        return ConversionError("Field 'y' is not a valid double");

    proto_out.set_x(x);
    proto_out.set_y(y);

    if (variant.contains("properties")) {
        auto props = variant["properties"].toMap();
        for (auto it = props.begin(); it != props.end(); ++it) {
            (*proto_out.mutable_properties())[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    return ConversionError();
}

ConversionError variantMapToConnectionData(
    const QVariantMap &variant,
    cme::ConnectionData &proto_out)
{
    if (!variant.contains("id"))
        return ConversionError("Missing required field: id");
    if (!variant.contains("sourceId"))
        return ConversionError("Missing required field: sourceId");
    if (!variant.contains("targetId"))
        return ConversionError("Missing required field: targetId");

    proto_out.set_id(variant["id"].toString().toStdString());
    proto_out.set_source_id(variant["sourceId"].toString().toStdString());
    proto_out.set_target_id(variant["targetId"].toString().toStdString());

    if (variant.contains("properties")) {
        auto props = variant["properties"].toMap();
        for (auto it = props.begin(); it != props.end(); ++it) {
            (*proto_out.mutable_properties())[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    return ConversionError();
}

ConversionError variantMapToGraphSnapshot(
    const QVariantMap &variant,
    cme::GraphSnapshot &proto_out)
{
    if (variant.contains("graphId")) {
        proto_out.set_graph_id(variant["graphId"].toString().toStdString());
    }

    if (variant.contains("components")) {
        const auto comps = variant["components"].toList();
        for (const auto &comp_variant : comps) {
            auto *pb_comp = proto_out.add_components();
            auto err = variantMapToComponentData(comp_variant.toMap(), *pb_comp);
            if (err.has_error)
                return err;
        }
    }

    if (variant.contains("connections")) {
        const auto conns = variant["connections"].toList();
        for (const auto &conn_variant : conns) {
            auto *pb_conn = proto_out.add_connections();
            auto err = variantMapToConnectionData(conn_variant.toMap(), *pb_conn);
            if (err.has_error)
                return err;
        }
    }

    return ConversionError();
}

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap componentDataToVariantMap(
    const cme::ComponentData &proto)
{
    QVariantMap result;
    result["id"] = QString::fromStdString(proto.id());
    result["typeId"] = QString::fromStdString(proto.type_id());
    result["x"] = proto.x();
    result["y"] = proto.y();

    QVariantMap props;
    for (const auto &kv : proto.properties()) {
        props[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    }
    if (!props.isEmpty())
        result["properties"] = props;

    return result;
}

QVariantMap connectionDataToVariantMap(
    const cme::ConnectionData &proto)
{
    QVariantMap result;
    result["id"] = QString::fromStdString(proto.id());
    result["sourceId"] = QString::fromStdString(proto.source_id());
    result["targetId"] = QString::fromStdString(proto.target_id());

    QVariantMap props;
    for (const auto &kv : proto.properties()) {
        props[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    }
    if (!props.isEmpty())
        result["properties"] = props;

    return result;
}

QVariantMap graphSnapshotToVariantMap(
    const cme::GraphSnapshot &proto)
{
    QVariantMap result;

    if (!proto.graph_id().empty())
        result["graphId"] = QString::fromStdString(proto.graph_id());

    QVariantList comps;
    for (int i = 0; i < proto.components_size(); ++i)
        comps.append(componentDataToVariantMap(proto.components(i)));
    if (!comps.isEmpty())
        result["components"] = comps;

    QVariantList conns;
    for (int i = 0; i < proto.connections_size(); ++i)
        conns.append(connectionDataToVariantMap(proto.connections(i)));
    if (!conns.isEmpty())
        result["connections"] = conns;

    return result;
}

} // namespace cme::adapter
