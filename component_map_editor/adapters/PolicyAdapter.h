// component_map_editor/adapters/PolicyAdapter.h
//
// Bidirectional adapter between QVariantMap (legacy) policy contracts and protobuf.
//
// Key legacy string keys:
//   - Policy context: "sourceTypeId", "targetTypeId", "sourcePort", "targetPort", "extra"
//   - Type defaults: "typeId", "defaultProperties", "defaultX", "defaultY"
//   - Connection properties: "connectionId", "properties"

#pragma once

#include <QString>
#include <QVariantMap>

#include "AdapterCommon.h"
#include "policy.pb.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToConnectionPolicyContext(
    const QVariantMap &variant,
    cme::ConnectionPolicyContext &proto_out
);

ConversionError variantMapToComponentTypeDefaults(
    const QVariantMap &variant,
    cme::ComponentTypeDefaults &proto_out
);

ConversionError variantMapToConnectionProperties(
    const QVariantMap &variant,
    cme::ConnectionProperties &proto_out
);

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap connectionPolicyContextToVariantMap(
    const cme::ConnectionPolicyContext &proto
);

QVariantMap componentTypeDefaultsToVariantMap(
    const cme::ComponentTypeDefaults &proto
);

QVariantMap connectionPropertiesToVariantMap(
    const cme::ConnectionProperties &proto
);

} // namespace cme::adapter
