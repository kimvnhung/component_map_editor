// component_map_editor/adapters/ValidationAdapter.h
//
// Bidirectional adapter between QVariantMap (legacy) validation contracts and protobuf.
//
// Key string keys:
//   - Validation issue map (unified, T004/T005):
//       "code" (normalized UPPER_SNAKE_CASE), "severity" ("error"/"warning"/"info"),
//       "message", "path" (components/<id>... | connections/<id>... | graph),
//       optional "details" (QVariantMap<string,string>) and "remediation" (QString).
//     Legacy keys "componentId" / "connectionId" are also emitted for backward
//     compatibility with existing consumers; they are never required.
//   - Provider-internal keys (legacy, converted in ValidationService):
//       "entityType" ("graph"/"component"/"connection"), "entityId"
//   - Severity values: "error", "warning", "info"
//   - Graph snapshot: "components", "connections" (delegates to GraphAdapter)
//
// Normalized issue contract (specs/002-normalize-validation-issue): issues are
// carried as structured maps across every layer boundary (service, adapter,
// client, exporter, manager). Only the UI layer may format an issue to text.
// `path` uses stable entity identifiers, never array indices.

#pragma once

#include <QString>
#include <QVariantMap>

#include "AdapterCommon.h"
#include "services/DiagnosticCodeCatalog.h"
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

// ── Path convention (T016, shared by editor and mapped to runtime) ─────────

/// Build a stable validation `path` from provider entityType/entityId.
/// Convention: "components/<entityId>", "connections/<entityId>", or "graph".
/// Paths use entity identifiers only — never array indices — so they are
/// invariant under entity reordering (T018).
QString validationPathFor(const QString &entityType, const QString &entityId);

// Parse a unified "path" back into its entity id.
/// Returns the trailing entity id for "components/<id>..." and
/// "connections/<id>..." paths, and an empty string for "graph".
QString entityIdFromValidationPath(const QString &path);

} // namespace cme::adapter
