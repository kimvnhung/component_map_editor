// component_map_editor/adapters/ValidationAdapter.h
//
// Bidirectional adapter between QVariantMap (legacy) validation contracts and protobuf.
//
// Key legacy string keys:
//   - Validation issue map: "code", "severity", "message", "componentId", "connectionId"
//   - Severity values: "error", "warning", "info"
//   - Graph snapshot: "components", "connections" (delegates to GraphAdapter)

#pragma once

#include <QString>
#include <QVariantMap>

#include "AdapterCommon.h"
#include "validation.pb.h"
#include "graph.pb.h"

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

// Convert legacy QVariantMap snapshot to typed GraphSnapshot (Phase 5)
ConversionError variantMapToGraphSnapshotForValidation(
    const QVariantMap &snapshotMap,
    cme::GraphSnapshot &proto_out
);

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap validationIssueToVariantMap(
    const cme::ValidationIssue &proto
);

QVariantMap graphValidationResultToVariantMap(
    const cme::GraphValidationResult &proto
);

// Convert typed GraphSnapshot to legacy QVariantMap for extensions (Phase 5)
QVariantMap graphSnapshotForValidationToVariantMap(
    const cme::GraphSnapshot &proto
);

// ── Severity helpers (Phase 8) ────────────────────────────────────────────

/// Convert a ValidationSeverity enum to its legacy string ("error"/"warning"/"info").
/// Use this constant in core code instead of hardcoding the raw string literal.
QString validationSeverityToString(cme::ValidationSeverity severity);

} // namespace cme::adapter
