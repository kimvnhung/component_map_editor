// component_map_editor/adapters/ExecutionAdapter.h
//
// Bidirectional adapter between QVariantMap (legacy) execution contracts and protobuf.
//
// Key legacy string keys:
//   - Timeline event: "type", "componentId", "timestamp", "message"  
//   - Component state: "componentId", "inputState", "outputState", "trace"
//   - Execution snapshot: "events", "componentStates", "sessionId"

#pragma once

#include <QString>
#include <QVariantMap>

#include "AdapterCommon.h"
#include "execution.pb.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToTimelineEvent(
    const QVariantMap &variant,
    cme::TimelineEvent &proto_out
);

ConversionError variantMapToComponentExecutionState(
    const QVariantMap &variant,
    cme::ComponentExecutionState &proto_out
);

ConversionError variantMapToExecutionSnapshot(
    const QVariantMap &variant,
    cme::ExecutionSnapshot &proto_out
);

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap timelineEventToVariantMap(
    const cme::TimelineEvent &proto
);

QVariantMap componentExecutionStateToVariantMap(
    const cme::ComponentExecutionState &proto
);

QVariantMap executionSnapshotToVariantMap(
    const cme::ExecutionSnapshot &proto
);

} // namespace cme::adapter
