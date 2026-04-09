// component_map_editor/adapters/ExecutionAdapter.cpp

#include "ExecutionAdapter.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToTimelineEvent(
    const QVariantMap &variant,
    cme::TimelineEvent &proto_out)
{
    // Determine event type by string
    QString type_str = variant.value("type", "").toString().toLower();
    if (type_str == "simulationstarted") {
        proto_out.set_type(cme::TIMELINE_EVENT_TYPE_SIMULATION_STARTED);
    } else if (type_str == "simulationcompleted") {
        proto_out.set_type(cme::TIMELINE_EVENT_TYPE_SIMULATION_COMPLETED);
    } else if (type_str == "simulationblocked") {
        proto_out.set_type(cme::TIMELINE_EVENT_TYPE_SIMULATION_BLOCKED);
    } else if (type_str == "breakpointhit") {
        proto_out.set_type(cme::TIMELINE_EVENT_TYPE_BREAKPOINT_HIT);
    } else if (type_str == "error") {
        proto_out.set_type(cme::TIMELINE_EVENT_TYPE_ERROR);
    } else if (type_str == "stepexecuted") {
        proto_out.set_type(cme::TIMELINE_EVENT_TYPE_STEP_EXECUTED);
    } else if (type_str == "simulationpaused") {
        proto_out.set_type(cme::TIMELINE_EVENT_TYPE_SIMULATION_PAUSED);
    } else {
        return ConversionError("Invalid timeline event type: " + type_str);
    }

    if (variant.contains("componentId"))
        proto_out.set_component_id(variant["componentId"].toString().toStdString());

    bool ok = false;
    qint64 ts = variant.value("timestamp", 0LL).toLongLong(&ok);
    if (!ok && variant.contains("timestamp"))
        return ConversionError("Field 'timestamp' is not a valid integer");
    proto_out.set_timestamp_ms(ts);

    if (variant.contains("message"))
        proto_out.set_message(variant["message"].toString().toStdString());

    return ConversionError();
}

ConversionError variantMapToComponentExecutionState(
    const QVariantMap &variant,
    cme::ComponentExecutionState &proto_out)
{
    if (!variant.contains("componentId"))
        return ConversionError("Missing required field: componentId");

    proto_out.set_component_id(variant["componentId"].toString().toStdString());

    if (variant.contains("inputState")) {
        auto input = variant["inputState"].toMap();
        for (auto it = input.begin(); it != input.end(); ++it) {
            (*proto_out.mutable_input_state())[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    if (variant.contains("outputState")) {
        auto output = variant["outputState"].toMap();
        for (auto it = output.begin(); it != output.end(); ++it) {
            (*proto_out.mutable_output_state())[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    if (variant.contains("trace"))
        proto_out.set_trace(variant["trace"].toString().toStdString());

    return ConversionError();
}

ConversionError variantMapToExecutionSnapshot(
    const QVariantMap &variant,
    cme::ExecutionSnapshot &proto_out)
{
    if (variant.contains("sessionId")) {
        proto_out.set_session_id(variant["sessionId"].toString().toStdString());
    }

    if (variant.contains("events")) {
        const auto events = variant["events"].toList();
        for (const auto &ev_variant : events) {
            auto *pb_ev = proto_out.add_events();
            auto err = variantMapToTimelineEvent(ev_variant.toMap(), *pb_ev);
            if (err.has_error)
                return err;
        }
    }

    if (variant.contains("componentStates")) {
        const auto states = variant["componentStates"].toList();
        for (const auto &state_variant : states) {
            auto *pb_state = proto_out.add_component_states();
            auto err = variantMapToComponentExecutionState(state_variant.toMap(), *pb_state);
            if (err.has_error)
                return err;
        }
    }

    return ConversionError();
}

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap timelineEventToVariantMap(
    const cme::TimelineEvent &proto)
{
    QVariantMap result;

    // Timeline event type enum -> string
    const char *type_str = "unknown";
    switch (proto.type()) {
    case cme::TIMELINE_EVENT_TYPE_SIMULATION_STARTED:
        type_str = "simulationStarted";
        break;
    case cme::TIMELINE_EVENT_TYPE_SIMULATION_COMPLETED:
        type_str = "simulationCompleted";
        break;
    case cme::TIMELINE_EVENT_TYPE_SIMULATION_BLOCKED:
        type_str = "simulationBlocked";
        break;
    case cme::TIMELINE_EVENT_TYPE_BREAKPOINT_HIT:
        type_str = "breakpointHit";
        break;
    case cme::TIMELINE_EVENT_TYPE_ERROR:
        type_str = "error";
        break;
    case cme::TIMELINE_EVENT_TYPE_STEP_EXECUTED:
        type_str = "stepExecuted";
        break;
    case cme::TIMELINE_EVENT_TYPE_SIMULATION_PAUSED:
        type_str = "simulationPaused";
        break;
    default:
        break;
    }
    result["type"] = QString::fromUtf8(type_str);

    if (!proto.component_id().empty())
        result["componentId"] = QString::fromStdString(proto.component_id());

    if (proto.timestamp_ms() != 0)
        result["timestamp"] = static_cast<qint64>(proto.timestamp_ms());

    if (!proto.message().empty())
        result["message"] = QString::fromStdString(proto.message());

    return result;
}

QVariantMap componentExecutionStateToVariantMap(
    const cme::ComponentExecutionState &proto)
{
    QVariantMap result;
    result["componentId"] = QString::fromStdString(proto.component_id());

    QVariantMap input;
    for (const auto &kv : proto.input_state())
        input[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    if (!input.isEmpty())
        result["inputState"] = input;

    QVariantMap output;
    for (const auto &kv : proto.output_state())
        output[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
    if (!output.isEmpty())
        result["outputState"] = output;

    if (!proto.trace().empty())
        result["trace"] = QString::fromStdString(proto.trace());

    return result;
}

QVariantMap executionSnapshotToVariantMap(
    const cme::ExecutionSnapshot &proto)
{
    QVariantMap result;

    if (!proto.session_id().empty())
        result["sessionId"] = QString::fromStdString(proto.session_id());

    QVariantList events;
    for (int i = 0; i < proto.events_size(); ++i)
        events.append(timelineEventToVariantMap(proto.events(i)));
    if (!events.isEmpty())
        result["events"] = events;

    QVariantList states;
    for (int i = 0; i < proto.component_states_size(); ++i)
        states.append(componentExecutionStateToVariantMap(proto.component_states(i)));
    if (!states.isEmpty())
        result["componentStates"] = states;

    return result;
}

} // namespace cme::adapter
