// component_map_editor/adapters/GraphAdapter.h
//
// Bidirectional adapter between QVariantMap (legacy) graph snapshots and protobuf.
//
// Key legacy string keys:
//   - Component: "id", "typeId", "x", "y", "properties"
//   - Connection: "id", "sourceId", "targetId", "properties"
//   - Graph: "graphId", "components", "connections"

#pragma once

#include <QString>
#include <QVariantMap>

#include "AdapterCommon.h"
#include "graph.pb.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToComponentData(
    const QVariantMap &variant,
    cme::ComponentData &proto_out
);

ConversionError variantMapToConnectionData(
    const QVariantMap &variant,
    cme::ConnectionData &proto_out
);

ConversionError variantMapToGraphSnapshot(
    const QVariantMap &variant,
    cme::GraphSnapshot &proto_out
);

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap componentDataToVariantMap(
    const cme::ComponentData &proto
);

QVariantMap connectionDataToVariantMap(
    const cme::ConnectionData &proto
);

QVariantMap graphSnapshotToVariantMap(
    const cme::GraphSnapshot &proto
);

} // namespace cme::adapter
