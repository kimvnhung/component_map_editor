// component_map_editor/adapters/ValidationAdapter.h
//
// Bidirectional adapter between QVariantMap (legacy) validation contracts and protobuf.
//
// Key legacy string keys:
//   - Validation issue map: "code", "severity", "message", "componentId", "connectionId"
//   - Severity values: "error", "warning", "info"

#pragma once

#include <QString>
#include <QVariantMap>

#include "AdapterCommon.h"
#include "validation.pb.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToValidationIssue(
    const QVariantMap &variant,
    cme::ValidationIssue &proto_out
);

ConversionError variantMapToGraphValidationResult(
    const QVariantMap &variant,
    cme::GraphValidationResult &proto_out
);

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap validationIssueToVariantMap(
    const cme::ValidationIssue &proto
);

QVariantMap graphValidationResultToVariantMap(
    const cme::GraphValidationResult &proto
);

} // namespace cme::adapter
