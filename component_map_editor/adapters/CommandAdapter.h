// component_map_editor/adapters/CommandAdapter.h
//
// Bidirectional adapter between QVariantMap (legacy) and protobuf command contracts.
//
// Key legacy string keys:
//   - Command dispatch by string (type-discriminator):
//     "addComponent", "removeComponent", "moveComponent", 
//     "addConnection", "removeConnection", 
//     "setComponentProperty", "setConnectionProperty"
//   - Component fields: "typeId", "componentId", "x", "y"
//   - Connection fields: "sourceId", "targetId", "connectionId"
//   - Property fields: "key", "value"

#pragma once

#include <QString>
#include <QVariantMap>
#include <memory>

#include "AdapterCommon.h"
#include "command.pb.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

/// Converts a QVariantMap representing a graph command into a typed GraphCommandRequest.
/// Logs conversion errors for type mismatches and missing required fields.
/// Unknown keys in the map are silently ignored (forward compat).
ConversionError variantMapToGraphCommandRequest(
    const QVariantMap &variant,
    cme::GraphCommandRequest &proto_out
);

/// Helper: convert a QVariantMap to AddComponentRequest.
ConversionError variantMapToAddComponentRequest(
    const QVariantMap &variant,
    cme::AddComponentRequest &proto_out
);

/// Helper: convert a QVariantMap to RemoveComponentRequest.
ConversionError variantMapToRemoveComponentRequest(
    const QVariantMap &variant,
    cme::RemoveComponentRequest &proto_out
);

/// Helper: convert a QVariantMap to MoveComponentRequest.
ConversionError variantMapToMoveComponentRequest(
    const QVariantMap &variant,
    cme::MoveComponentRequest &proto_out
);

/// Helper: convert a QVariantMap to AddConnectionRequest.
ConversionError variantMapToAddConnectionRequest(
    const QVariantMap &variant,
    cme::AddConnectionRequest &proto_out
);

/// Helper: convert a QVariantMap to RemoveConnectionRequest.
ConversionError variantMapToRemoveConnectionRequest(
    const QVariantMap &variant,
    cme::RemoveConnectionRequest &proto_out
);

/// Helper: convert a QVariantMap to SetComponentPropertyRequest.
ConversionError variantMapToSetComponentPropertyRequest(
    const QVariantMap &variant,
    cme::SetComponentPropertyRequest &proto_out
);

/// Helper: convert a QVariantMap to SetConnectionPropertyRequest.
ConversionError variantMapToSetConnectionPropertyRequest(
    const QVariantMap &variant,
    cme::SetConnectionPropertyRequest &proto_out
);

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

/// Converts a typed GraphCommandRequest into a QVariantMap for legacy code.
/// Restores the command type as a string discriminator for backward compatibility.
QVariantMap graphCommandRequestToVariantMap(
    const cme::GraphCommandRequest &proto
);

/// Helper: convert AddComponentRequest to QVariantMap.
QVariantMap addComponentRequestToVariantMap(
    const cme::AddComponentRequest &proto
);

/// Helper: convert RemoveComponentRequest to QVariantMap.
QVariantMap removeComponentRequestToVariantMap(
    const cme::RemoveComponentRequest &proto
);

/// Helper: convert MoveComponentRequest to QVariantMap.
QVariantMap moveComponentRequestToVariantMap(
    const cme::MoveComponentRequest &proto
);

/// Helper: convert AddConnectionRequest to QVariantMap.
QVariantMap addConnectionRequestToVariantMap(
    const cme::AddConnectionRequest &proto
);

/// Helper: convert RemoveConnectionRequest to QVariantMap.
QVariantMap removeConnectionRequestToVariantMap(
    const cme::RemoveConnectionRequest &proto
);

/// Helper: convert SetComponentPropertyRequest to QVariantMap.
QVariantMap setComponentPropertyRequestToVariantMap(
    const cme::SetComponentPropertyRequest &proto
);

/// Helper: convert SetConnectionPropertyRequest to QVariantMap.
QVariantMap setConnectionPropertyRequestToVariantMap(
    const cme::SetConnectionPropertyRequest &proto
);

} // namespace cme::adapter
