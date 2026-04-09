// component_map_editor/adapters/CommandAdapter.cpp

#include "CommandAdapter.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToGraphCommandRequest(
    const QVariantMap &variant,
    cme::GraphCommandRequest &proto_out)
{
    // Determine command type by inspecting keys
    if (variant.contains("addComponent")) {
        auto *payload = proto_out.mutable_add_component();
        return variantMapToAddComponentRequest(variant["addComponent"].toMap(), *payload);
    }
    if (variant.contains("removeComponent")) {
        auto *payload = proto_out.mutable_remove_component();
        return variantMapToRemoveComponentRequest(variant["removeComponent"].toMap(), *payload);
    }
    if (variant.contains("moveComponent")) {
        auto *payload = proto_out.mutable_move_component();
        return variantMapToMoveComponentRequest(variant["moveComponent"].toMap(), *payload);
    }
    if (variant.contains("addConnection")) {
        auto *payload = proto_out.mutable_add_connection();
        return variantMapToAddConnectionRequest(variant["addConnection"].toMap(), *payload);
    }
    if (variant.contains("removeConnection")) {
        auto *payload = proto_out.mutable_remove_connection();
        return variantMapToRemoveConnectionRequest(variant["removeConnection"].toMap(), *payload);
    }
    if (variant.contains("setComponentProperty")) {
        auto *payload = proto_out.mutable_set_component_property();
        return variantMapToSetComponentPropertyRequest(variant["setComponentProperty"].toMap(), *payload);
    }
    if (variant.contains("setConnectionProperty")) {
        auto *payload = proto_out.mutable_set_connection_property();
        return variantMapToSetConnectionPropertyRequest(variant["setConnectionProperty"].toMap(), *payload);
    }

    return ConversionError("Command type not recognized (missing addComponent/removeComponent/etc.)");
}

ConversionError variantMapToAddComponentRequest(
    const QVariantMap &variant,
    cme::AddComponentRequest &proto_out)
{
    if (!variant.contains("typeId"))
        return ConversionError("Missing required field: typeId");
    if (!variant.contains("x"))
        return ConversionError("Missing required field: x");
    if (!variant.contains("y"))
        return ConversionError("Missing required field: y");

    bool ok_x = false, ok_y = false;
    double x = variant["x"].toDouble(&ok_x);
    double y = variant["y"].toDouble(&ok_y);

    if (!ok_x)
        return ConversionError("Field 'x' is not a valid double");
    if (!ok_y)
        return ConversionError("Field 'y' is not a valid double");

    proto_out.set_type_id(variant["typeId"].toString().toStdString());
    proto_out.set_x(x);
    proto_out.set_y(y);
    if (variant.contains("componentId"))
        proto_out.set_component_id(variant["componentId"].toString().toStdString());

    return ConversionError();
}

ConversionError variantMapToRemoveComponentRequest(
    const QVariantMap &variant,
    cme::RemoveComponentRequest &proto_out)
{
    if (!variant.contains("componentId"))
        return ConversionError("Missing required field: componentId");

    proto_out.set_component_id(variant["componentId"].toString().toStdString());
    return ConversionError();
}

ConversionError variantMapToMoveComponentRequest(
    const QVariantMap &variant,
    cme::MoveComponentRequest &proto_out)
{
    if (!variant.contains("componentId"))
        return ConversionError("Missing required field: componentId");
    if (!variant.contains("oldX"))
        return ConversionError("Missing required field: oldX");
    if (!variant.contains("oldY"))
        return ConversionError("Missing required field: oldY");
    if (!variant.contains("newX"))
        return ConversionError("Missing required field: newX");
    if (!variant.contains("newY"))
        return ConversionError("Missing required field: newY");

    bool ok_oldx = false, ok_oldy = false, ok_newx = false, ok_newy = false;
    double old_x = variant["oldX"].toDouble(&ok_oldx);
    double old_y = variant["oldY"].toDouble(&ok_oldy);
    double new_x = variant["newX"].toDouble(&ok_newx);
    double new_y = variant["newY"].toDouble(&ok_newy);

    if (!ok_oldx)
        return ConversionError("Field 'oldX' is not a valid double");
    if (!ok_oldy)
        return ConversionError("Field 'oldY' is not a valid double");
    if (!ok_newx)
        return ConversionError("Field 'newX' is not a valid double");
    if (!ok_newy)
        return ConversionError("Field 'newY' is not a valid double");

    proto_out.set_component_id(variant["componentId"].toString().toStdString());
    proto_out.set_old_x(old_x);
    proto_out.set_old_y(old_y);
    proto_out.set_new_x(new_x);
    proto_out.set_new_y(new_y);

    return ConversionError();
}

ConversionError variantMapToAddConnectionRequest(
    const QVariantMap &variant,
    cme::AddConnectionRequest &proto_out)
{
    if (!variant.contains("sourceId"))
        return ConversionError("Missing required field: sourceId");
    if (!variant.contains("targetId"))
        return ConversionError("Missing required field: targetId");

    proto_out.set_source_id(variant["sourceId"].toString().toStdString());
    proto_out.set_target_id(variant["targetId"].toString().toStdString());
    if (variant.contains("connectionId"))
        proto_out.set_connection_id(variant["connectionId"].toString().toStdString());

    return ConversionError();
}

ConversionError variantMapToRemoveConnectionRequest(
    const QVariantMap &variant,
    cme::RemoveConnectionRequest &proto_out)
{
    if (!variant.contains("connectionId"))
        return ConversionError("Missing required field: connectionId");

    proto_out.set_connection_id(variant["connectionId"].toString().toStdString());
    return ConversionError();
}

ConversionError variantMapToSetComponentPropertyRequest(
    const QVariantMap &variant,
    cme::SetComponentPropertyRequest &proto_out)
{
    if (!variant.contains("componentId"))
        return ConversionError("Missing required field: componentId");
    if (!variant.contains("key"))
        return ConversionError("Missing required field: key");
    if (!variant.contains("value"))
        return ConversionError("Missing required field: value");

    proto_out.set_component_id(variant["componentId"].toString().toStdString());
    proto_out.set_key(variant["key"].toString().toStdString());
    proto_out.set_value(variant["value"].toString().toStdString());

    return ConversionError();
}

ConversionError variantMapToSetConnectionPropertyRequest(
    const QVariantMap &variant,
    cme::SetConnectionPropertyRequest &proto_out)
{
    if (!variant.contains("connectionId"))
        return ConversionError("Missing required field: connectionId");
    if (!variant.contains("key"))
        return ConversionError("Missing required field: key");
    if (!variant.contains("value"))
        return ConversionError("Missing required field: value");

    proto_out.set_connection_id(variant["connectionId"].toString().toStdString());
    proto_out.set_key(variant["key"].toString().toStdString());
    proto_out.set_value(variant["value"].toString().toStdString());

    return ConversionError();
}

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap graphCommandRequestToVariantMap(
    const cme::GraphCommandRequest &proto)
{
    QVariantMap result;

    switch (proto.payload_case()) {
    case cme::GraphCommandRequest::kAddComponent:
        result["addComponent"] = addComponentRequestToVariantMap(proto.add_component());
        break;
    case cme::GraphCommandRequest::kRemoveComponent:
        result["removeComponent"] = removeComponentRequestToVariantMap(proto.remove_component());
        break;
    case cme::GraphCommandRequest::kMoveComponent:
        result["moveComponent"] = moveComponentRequestToVariantMap(proto.move_component());
        break;
    case cme::GraphCommandRequest::kAddConnection:
        result["addConnection"] = addConnectionRequestToVariantMap(proto.add_connection());
        break;
    case cme::GraphCommandRequest::kRemoveConnection:
        result["removeConnection"] = removeConnectionRequestToVariantMap(proto.remove_connection());
        break;
    case cme::GraphCommandRequest::kSetComponentProperty:
        result["setComponentProperty"] = setComponentPropertyRequestToVariantMap(proto.set_component_property());
        break;
    case cme::GraphCommandRequest::kSetConnectionProperty:
        result["setConnectionProperty"] = setConnectionPropertyRequestToVariantMap(proto.set_connection_property());
        break;
    case cme::GraphCommandRequest::PAYLOAD_NOT_SET:
        break;
    }

    return result;
}

QVariantMap addComponentRequestToVariantMap(
    const cme::AddComponentRequest &proto)
{
    QVariantMap result;
    result["typeId"] = QString::fromStdString(proto.type_id());
    result["x"] = proto.x();
    result["y"] = proto.y();
    if (!proto.component_id().empty())
        result["componentId"] = QString::fromStdString(proto.component_id());
    return result;
}

QVariantMap removeComponentRequestToVariantMap(
    const cme::RemoveComponentRequest &proto)
{
    QVariantMap result;
    result["componentId"] = QString::fromStdString(proto.component_id());
    return result;
}

QVariantMap moveComponentRequestToVariantMap(
    const cme::MoveComponentRequest &proto)
{
    QVariantMap result;
    result["componentId"] = QString::fromStdString(proto.component_id());
    result["oldX"] = proto.old_x();
    result["oldY"] = proto.old_y();
    result["newX"] = proto.new_x();
    result["newY"] = proto.new_y();
    return result;
}

QVariantMap addConnectionRequestToVariantMap(
    const cme::AddConnectionRequest &proto)
{
    QVariantMap result;
    result["sourceId"] = QString::fromStdString(proto.source_id());
    result["targetId"] = QString::fromStdString(proto.target_id());
    if (!proto.connection_id().empty())
        result["connectionId"] = QString::fromStdString(proto.connection_id());
    return result;
}

QVariantMap removeConnectionRequestToVariantMap(
    const cme::RemoveConnectionRequest &proto)
{
    QVariantMap result;
    result["connectionId"] = QString::fromStdString(proto.connection_id());
    return result;
}

QVariantMap setComponentPropertyRequestToVariantMap(
    const cme::SetComponentPropertyRequest &proto)
{
    QVariantMap result;
    result["componentId"] = QString::fromStdString(proto.component_id());
    result["key"] = QString::fromStdString(proto.key());
    result["value"] = QString::fromStdString(proto.value());
    return result;
}

QVariantMap setConnectionPropertyRequestToVariantMap(
    const cme::SetConnectionPropertyRequest &proto)
{
    QVariantMap result;
    result["connectionId"] = QString::fromStdString(proto.connection_id());
    result["key"] = QString::fromStdString(proto.key());
    result["value"] = QString::fromStdString(proto.value());
    return result;
}

// ── Command type string ↔ enum conversion (Phase 8) ──────────────────────────
// All string literals for command dispatch are confined to this adapter.
// Core modules (CommandGateway) must use cme::CommandType enum for branching.

cme::CommandType commandTypeFromString(const QString &commandType)
{
    if (commandType == QLatin1String("addComponent"))          return cme::COMMAND_TYPE_ADD_COMPONENT;
    if (commandType == QLatin1String("removeComponent"))       return cme::COMMAND_TYPE_REMOVE_COMPONENT;
    if (commandType == QLatin1String("moveComponent"))         return cme::COMMAND_TYPE_MOVE_COMPONENT;
    if (commandType == QLatin1String("addConnection"))         return cme::COMMAND_TYPE_ADD_CONNECTION;
    if (commandType == QLatin1String("removeConnection"))      return cme::COMMAND_TYPE_REMOVE_CONNECTION;
    if (commandType == QLatin1String("setComponentProperty"))  return cme::COMMAND_TYPE_SET_COMPONENT_PROPERTY;
    if (commandType == QLatin1String("setConnectionProperty")) return cme::COMMAND_TYPE_SET_CONNECTION_PROPERTY;
    return cme::COMMAND_TYPE_UNSPECIFIED;
}

QString commandTypeToString(cme::CommandType commandType)
{
    switch (commandType) {
    case cme::COMMAND_TYPE_ADD_COMPONENT:           return QStringLiteral("addComponent");
    case cme::COMMAND_TYPE_REMOVE_COMPONENT:        return QStringLiteral("removeComponent");
    case cme::COMMAND_TYPE_MOVE_COMPONENT:          return QStringLiteral("moveComponent");
    case cme::COMMAND_TYPE_ADD_CONNECTION:          return QStringLiteral("addConnection");
    case cme::COMMAND_TYPE_REMOVE_CONNECTION:       return QStringLiteral("removeConnection");
    case cme::COMMAND_TYPE_SET_COMPONENT_PROPERTY:  return QStringLiteral("setComponentProperty");
    case cme::COMMAND_TYPE_SET_CONNECTION_PROPERTY: return QStringLiteral("setConnectionProperty");
    default:                                        return {};
    }
}

} // namespace cme::adapter
