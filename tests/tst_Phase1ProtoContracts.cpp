// tst_Phase1ProtoContracts.cpp
//
// Phase 1: Protobuf Contract Foundation
//
// Verifies that every core proto message survives a serialize → parse
// roundtrip, that all required fields are preserved, and that enum values
// match the string keys frozen in tst_Phase0Baseline.
//
// Pass condition: all roundtrip tests green, build is clean with generated
// *.pb.cc/.pb.h from protoc 3.21.12.

#include <QtTest>

#include "command.pb.h"
#include "execution.pb.h"
#include "graph.pb.h"
#include "policy.pb.h"
#include "public_api.pb.h"
#include "provider_templates.pb.h"
#include "validation.pb.h"

// ── helpers ────────────────────────────────────────────────────────────────
namespace {

template <typename T>
T roundtrip(const T &msg) {
    std::string buf;
    if (!msg.SerializeToString(&buf))
        qFatal("SerializeToString failed for message type");
    T decoded;
    if (!decoded.ParseFromString(buf))
        qFatal("ParseFromString failed for message type");
    return decoded;
}

} // anonymous namespace

class tst_Phase1ProtoContracts : public QObject {
    Q_OBJECT

private Q_SLOTS:

    // ── command.proto ────────────────────────────────────────────────────
    void addComponentRequest_roundtrip();
    void removeComponentRequest_roundtrip();
    void moveComponentRequest_roundtrip();
    void addConnectionRequest_roundtrip();
    void removeConnectionRequest_roundtrip();
    void setComponentPropertyRequest_roundtrip();
    void setConnectionPropertyRequest_roundtrip();
    void graphCommandResult_success_roundtrip();
    void graphCommandResult_failure_roundtrip();
    void errorCodeEnumValues();

    // ── graph.proto ──────────────────────────────────────────────────────
    void componentData_roundtrip();
    void connectionData_roundtrip();
    void graphSnapshot_roundtrip();

    // ── validation.proto ─────────────────────────────────────────────────
    void validationIssue_errorSeverity_roundtrip();
    void validationIssue_warningSeverity_roundtrip();
    void graphValidationResult_roundtrip();
    void validationSeverityEnumValues();

    // ── execution.proto ──────────────────────────────────────────────────
    void timelineEvent_roundtrip();
    void executionSnapshot_roundtrip();
    void timelineEventTypeEnumValues();

    // ── policy.proto ─────────────────────────────────────────────────────
    void connectionPolicyContext_roundtrip();
    void componentTypeDefaults_roundtrip();
    void connectionProperties_roundtrip();

    // ── public_api.proto ────────────────────────────────────────────────
    void actionListRequest_roundtrip();
    void actionListResponse_roundtrip();
    void actionDescriptorRequest_roundtrip();
    void actionDescriptorResponse_roundtrip();
    void actionInvokeRequest_roundtrip();
    void actionInvokeResponse_roundtrip();
    void componentTypeIdsRequest_roundtrip();
    void componentTypeIdsResponse_roundtrip();
    void componentTypeDescriptorRequest_roundtrip();
    void componentTypeDescriptorResponse_roundtrip();
    void defaultComponentPropertiesRequest_roundtrip();
    void defaultComponentPropertiesResponse_roundtrip();
    void canConnectRequest_roundtrip();
    void canConnectResponse_roundtrip();
    void normalizeConnectionPropertiesRequest_roundtrip();
    void normalizeConnectionPropertiesResponse_roundtrip();
    void supportedComponentTypesRequest_roundtrip();
    void supportedComponentTypesResponse_roundtrip();
    void executeComponentRequest_roundtrip();
    void executeComponentResponse_roundtrip();
    void schemaTargetsRequest_roundtrip();
    void schemaTargetsResponse_roundtrip();
    void propertySchemaRequest_roundtrip();
    void propertySchemaResponse_roundtrip();
    void validateGraphRequest_roundtrip();
    void validateGraphResponse_roundtrip();
    void executionSnapshotEnvelope_roundtrip();

    // ── provider_templates.proto ───────────────────────────────────────
    void propertySchemaFieldTemplate_roundtrip();
    void propertySchemaTemplateBundle_roundtrip();
    void componentTypeTemplateBundle_roundtrip();
    void connectionPolicyTemplateBundle_roundtrip();
};

// ============================================================================
// command.proto
// ============================================================================

void tst_Phase1ProtoContracts::addComponentRequest_roundtrip()
{
    cme::GraphCommandRequest req;
    auto *p = req.mutable_add_component();
    p->set_type_id("logic/AndGate");
    p->set_x(100.5);
    p->set_y(200.75);
    p->set_component_id("comp-001");

    auto decoded = roundtrip(req);
    QVERIFY(decoded.has_add_component());
    QCOMPARE(decoded.add_component().type_id(),      std::string("logic/AndGate"));
    QCOMPARE(decoded.add_component().x(),            100.5);
    QCOMPARE(decoded.add_component().y(),            200.75);
    QCOMPARE(decoded.add_component().component_id(), std::string("comp-001"));
    QCOMPARE(decoded.payload_case(),
             cme::GraphCommandRequest::kAddComponent);
}

void tst_Phase1ProtoContracts::removeComponentRequest_roundtrip()
{
    cme::GraphCommandRequest req;
    req.mutable_remove_component()->set_component_id("comp-007");

    auto decoded = roundtrip(req);
    QVERIFY(decoded.has_remove_component());
    QCOMPARE(decoded.remove_component().component_id(), std::string("comp-007"));
    QCOMPARE(decoded.payload_case(),
             cme::GraphCommandRequest::kRemoveComponent);
}

void tst_Phase1ProtoContracts::moveComponentRequest_roundtrip()
{
    cme::GraphCommandRequest req;
    auto *p = req.mutable_move_component();
    p->set_component_id("comp-003");
    p->set_old_x(10.0);
    p->set_old_y(20.0);
    p->set_new_x(30.0);
    p->set_new_y(40.0);

    auto decoded = roundtrip(req);
    QVERIFY(decoded.has_move_component());
    QCOMPARE(decoded.move_component().component_id(), std::string("comp-003"));
    QCOMPARE(decoded.move_component().old_x(), 10.0);
    QCOMPARE(decoded.move_component().old_y(), 20.0);
    QCOMPARE(decoded.move_component().new_x(), 30.0);
    QCOMPARE(decoded.move_component().new_y(), 40.0);
}

void tst_Phase1ProtoContracts::addConnectionRequest_roundtrip()
{
    cme::GraphCommandRequest req;
    auto *p = req.mutable_add_connection();
    p->set_source_id("comp-001");
    p->set_target_id("comp-002");
    p->set_connection_id("conn-001");

    auto decoded = roundtrip(req);
    QVERIFY(decoded.has_add_connection());
    QCOMPARE(decoded.add_connection().source_id(),     std::string("comp-001"));
    QCOMPARE(decoded.add_connection().target_id(),     std::string("comp-002"));
    QCOMPARE(decoded.add_connection().connection_id(), std::string("conn-001"));
}

void tst_Phase1ProtoContracts::removeConnectionRequest_roundtrip()
{
    cme::GraphCommandRequest req;
    req.mutable_remove_connection()->set_connection_id("conn-099");

    auto decoded = roundtrip(req);
    QVERIFY(decoded.has_remove_connection());
    QCOMPARE(decoded.remove_connection().connection_id(), std::string("conn-099"));
}

void tst_Phase1ProtoContracts::setComponentPropertyRequest_roundtrip()
{
    cme::GraphCommandRequest req;
    auto *p = req.mutable_set_component_property();
    p->set_component_id("comp-001");
    p->set_key("label");
    p->set_value("MyGate");

    auto decoded = roundtrip(req);
    QVERIFY(decoded.has_set_component_property());
    QCOMPARE(decoded.set_component_property().component_id(), std::string("comp-001"));
    QCOMPARE(decoded.set_component_property().key(),          std::string("label"));
    QCOMPARE(decoded.set_component_property().value(),        std::string("MyGate"));
}

void tst_Phase1ProtoContracts::setConnectionPropertyRequest_roundtrip()
{
    cme::GraphCommandRequest req;
    auto *p = req.mutable_set_connection_property();
    p->set_connection_id("conn-001");
    p->set_key("weight");
    p->set_value("1.5");

    auto decoded = roundtrip(req);
    QVERIFY(decoded.has_set_connection_property());
    QCOMPARE(decoded.set_connection_property().connection_id(), std::string("conn-001"));
    QCOMPARE(decoded.set_connection_property().key(),           std::string("weight"));
    QCOMPARE(decoded.set_connection_property().value(),         std::string("1.5"));
}

void tst_Phase1ProtoContracts::graphCommandResult_success_roundtrip()
{
    cme::GraphCommandResult result;
    result.set_success(true);
    result.set_error_code(cme::ERROR_CODE_UNSPECIFIED);

    auto decoded = roundtrip(result);
    QVERIFY(decoded.success());
    QCOMPARE(decoded.error_code(), cme::ERROR_CODE_UNSPECIFIED);
    QVERIFY(decoded.error_message().empty());
}

void tst_Phase1ProtoContracts::graphCommandResult_failure_roundtrip()
{
    cme::GraphCommandResult result;
    result.set_success(false);
    result.set_error_code(cme::ERROR_CODE_MISSING_FIELD);
    result.set_error_message("field 'typeId' is required");

    auto decoded = roundtrip(result);
    QVERIFY(!decoded.success());
    QCOMPARE(decoded.error_code(), cme::ERROR_CODE_MISSING_FIELD);
    QCOMPARE(decoded.error_message(), std::string("field 'typeId' is required"));
}

void tst_Phase1ProtoContracts::errorCodeEnumValues()
{
    // Enum numeric values must remain stable across proto file edits.
    QCOMPARE(static_cast<int>(cme::ERROR_CODE_UNSPECIFIED),            0);
    QCOMPARE(static_cast<int>(cme::ERROR_CODE_UNKNOWN_COMMAND),        1);
    QCOMPARE(static_cast<int>(cme::ERROR_CODE_MISSING_FIELD),          2);
    QCOMPARE(static_cast<int>(cme::ERROR_CODE_CAPABILITY_DENIED),      3);
    QCOMPARE(static_cast<int>(cme::ERROR_CODE_POST_INVARIANT_VIOLATED), 4);
    QCOMPARE(static_cast<int>(cme::ERROR_CODE_POLICY_REJECTED),        5);
}

// ============================================================================
// graph.proto
// ============================================================================

void tst_Phase1ProtoContracts::componentData_roundtrip()
{
    cme::ComponentData comp;
    comp.set_id("comp-001");
    comp.set_type_id("logic/AndGate");
    comp.set_x(50.0);
    comp.set_y(75.0);
    (*comp.mutable_properties())["label"] = "Gate A";
    (*comp.mutable_properties())["inputs"] = "2";

    auto decoded = roundtrip(comp);
    QCOMPARE(decoded.id(),      std::string("comp-001"));
    QCOMPARE(decoded.type_id(), std::string("logic/AndGate"));
    QCOMPARE(decoded.x(),       50.0);
    QCOMPARE(decoded.y(),       75.0);
    QCOMPARE(decoded.properties().count("label"),  1);
    QCOMPARE(decoded.properties().at("label"),     std::string("Gate A"));
    QCOMPARE(decoded.properties().at("inputs"),    std::string("2"));
}

void tst_Phase1ProtoContracts::connectionData_roundtrip()
{
    cme::ConnectionData conn;
    conn.set_id("conn-001");
    conn.set_source_id("comp-001");
    conn.set_target_id("comp-002");
    (*conn.mutable_properties())["capacity"] = "10";

    auto decoded = roundtrip(conn);
    QCOMPARE(decoded.id(),        std::string("conn-001"));
    QCOMPARE(decoded.source_id(), std::string("comp-001"));
    QCOMPARE(decoded.target_id(), std::string("comp-002"));
    QCOMPARE(decoded.properties().at("capacity"), std::string("10"));
}

void tst_Phase1ProtoContracts::graphSnapshot_roundtrip()
{
    cme::GraphSnapshot snap;
    snap.set_graph_id("graph-42");

    auto *c = snap.add_components();
    c->set_id("c1");
    c->set_type_id("typeA");
    c->set_x(1.0);
    c->set_y(2.0);

    auto *e = snap.add_connections();
    e->set_id("e1");
    e->set_source_id("c1");
    e->set_target_id("c2");

    auto decoded = roundtrip(snap);
    QCOMPARE(decoded.graph_id(),           std::string("graph-42"));
    QCOMPARE(decoded.components_size(),    1);
    QCOMPARE(decoded.connections_size(),   1);
    QCOMPARE(decoded.components(0).id(),   std::string("c1"));
    QCOMPARE(decoded.connections(0).id(),  std::string("e1"));
}

// ============================================================================
// validation.proto
// ============================================================================

void tst_Phase1ProtoContracts::validationIssue_errorSeverity_roundtrip()
{
    cme::ValidationIssue issue;
    issue.set_code("CORE_NULL_GRAPH");
    issue.set_severity(cme::VALIDATION_SEVERITY_ERROR);
    issue.set_message("Graph snapshot is null or empty");

    auto decoded = roundtrip(issue);
    QCOMPARE(decoded.code(),     std::string("CORE_NULL_GRAPH"));
    QCOMPARE(decoded.severity(), cme::VALIDATION_SEVERITY_ERROR);
    QCOMPARE(decoded.message(),  std::string("Graph snapshot is null or empty"));
    QVERIFY(decoded.component_id().empty());
    QVERIFY(decoded.connection_id().empty());
}

void tst_Phase1ProtoContracts::validationIssue_warningSeverity_roundtrip()
{
    cme::ValidationIssue issue;
    issue.set_code("DANGLING_NODE");
    issue.set_severity(cme::VALIDATION_SEVERITY_WARNING);
    issue.set_component_id("comp-007");

    auto decoded = roundtrip(issue);
    QCOMPARE(decoded.code(),         std::string("DANGLING_NODE"));
    QCOMPARE(decoded.severity(),     cme::VALIDATION_SEVERITY_WARNING);
    QCOMPARE(decoded.component_id(), std::string("comp-007"));
}

void tst_Phase1ProtoContracts::graphValidationResult_roundtrip()
{
    cme::GraphValidationResult result;
    result.set_is_valid(false);
    auto *issue = result.add_issues();
    issue->set_code("MISSING_SINK");
    issue->set_severity(cme::VALIDATION_SEVERITY_ERROR);

    auto decoded = roundtrip(result);
    QVERIFY(!decoded.is_valid());
    QCOMPARE(decoded.issues_size(), 1);
    QCOMPARE(decoded.issues(0).code(), std::string("MISSING_SINK"));
    QCOMPARE(decoded.issues(0).severity(), cme::VALIDATION_SEVERITY_ERROR);
}

void tst_Phase1ProtoContracts::validationSeverityEnumValues()
{
    QCOMPARE(static_cast<int>(cme::VALIDATION_SEVERITY_UNSPECIFIED), 0);
    QCOMPARE(static_cast<int>(cme::VALIDATION_SEVERITY_ERROR),       1);
    QCOMPARE(static_cast<int>(cme::VALIDATION_SEVERITY_WARNING),     2);
    QCOMPARE(static_cast<int>(cme::VALIDATION_SEVERITY_INFO),        3);
}

// ============================================================================
// execution.proto
// ============================================================================

void tst_Phase1ProtoContracts::timelineEvent_roundtrip()
{
    cme::TimelineEvent ev;
    ev.set_type(cme::TIMELINE_EVENT_TYPE_SIMULATION_STARTED);
    ev.set_component_id("");
    ev.set_timestamp_ms(1234567890LL);
    ev.set_message("simulation started");

    auto decoded = roundtrip(ev);
    QCOMPARE(decoded.type(),         cme::TIMELINE_EVENT_TYPE_SIMULATION_STARTED);
    QCOMPARE(decoded.timestamp_ms(), 1234567890LL);
    QCOMPARE(decoded.message(),      std::string("simulation started"));
}

void tst_Phase1ProtoContracts::executionSnapshot_roundtrip()
{
    cme::ExecutionSnapshot snap;
    snap.set_session_id("session-001");

    {
        auto *ev = snap.add_events();
        ev->set_type(cme::TIMELINE_EVENT_TYPE_BREAKPOINT_HIT);
        ev->set_component_id("comp-5");
        ev->set_timestamp_ms(500LL);
    }
    {
        auto *cs = snap.add_component_states();
        cs->set_component_id("comp-5");
        (*cs->mutable_input_state())["in0"]  = "1";
        (*cs->mutable_output_state())["out0"] = "0";
        cs->set_trace("evaluated");
    }

    auto decoded = roundtrip(snap);
    QCOMPARE(decoded.session_id(),             std::string("session-001"));
    QCOMPARE(decoded.events_size(),            1);
    QCOMPARE(decoded.component_states_size(),  1);
    QCOMPARE(decoded.events(0).type(),         cme::TIMELINE_EVENT_TYPE_BREAKPOINT_HIT);
    QCOMPARE(decoded.events(0).component_id(), std::string("comp-5"));
    QCOMPARE(decoded.component_states(0).component_id(), std::string("comp-5"));
    QCOMPARE(decoded.component_states(0).input_state().at("in0"),   std::string("1"));
    QCOMPARE(decoded.component_states(0).output_state().at("out0"), std::string("0"));
    QCOMPARE(decoded.component_states(0).trace(), std::string("evaluated"));
}

void tst_Phase1ProtoContracts::timelineEventTypeEnumValues()
{
    // These numeric values mirror the frozen string names from Phase 0.
    QCOMPARE(static_cast<int>(cme::TIMELINE_EVENT_TYPE_UNSPECIFIED),          0);
    QCOMPARE(static_cast<int>(cme::TIMELINE_EVENT_TYPE_SIMULATION_STARTED),   1);
    QCOMPARE(static_cast<int>(cme::TIMELINE_EVENT_TYPE_SIMULATION_COMPLETED), 2);
    QCOMPARE(static_cast<int>(cme::TIMELINE_EVENT_TYPE_SIMULATION_BLOCKED),   3);
    QCOMPARE(static_cast<int>(cme::TIMELINE_EVENT_TYPE_BREAKPOINT_HIT),       4);
    QCOMPARE(static_cast<int>(cme::TIMELINE_EVENT_TYPE_ERROR),                5);
}

// ============================================================================
// policy.proto
// ============================================================================

void tst_Phase1ProtoContracts::connectionPolicyContext_roundtrip()
{
    cme::ConnectionPolicyContext ctx;
    ctx.set_source_type_id("logic/AndGate");
    ctx.set_target_type_id("logic/OrGate");
    ctx.set_source_port("out0");
    ctx.set_target_port("in1");
    (*ctx.mutable_extra())["bandwidth"] = "100";

    auto decoded = roundtrip(ctx);
    QCOMPARE(decoded.source_type_id(), std::string("logic/AndGate"));
    QCOMPARE(decoded.target_type_id(), std::string("logic/OrGate"));
    QCOMPARE(decoded.source_port(),    std::string("out0"));
    QCOMPARE(decoded.target_port(),    std::string("in1"));
    QCOMPARE(decoded.extra().at("bandwidth"), std::string("100"));
}

void tst_Phase1ProtoContracts::componentTypeDefaults_roundtrip()
{
    cme::ComponentTypeDefaults def;
    def.set_type_id("logic/AndGate");
    def.set_default_x(0.0);
    def.set_default_y(0.0);
    (*def.mutable_default_properties())["inputs"] = "2";
    (*def.mutable_default_properties())["label"]  = "";

    auto decoded = roundtrip(def);
    QCOMPARE(decoded.type_id(), std::string("logic/AndGate"));
    QCOMPARE(decoded.default_properties().at("inputs"), std::string("2"));
    QCOMPARE(decoded.default_x(), 0.0);
    QCOMPARE(decoded.default_y(), 0.0);
}

void tst_Phase1ProtoContracts::connectionProperties_roundtrip()
{
    cme::ConnectionProperties cp;
    cp.set_connection_id("conn-001");
    (*cp.mutable_properties())["capacity"] = "10";
    (*cp.mutable_properties())["latency"]  = "5ms";

    auto decoded = roundtrip(cp);
    QCOMPARE(decoded.connection_id(), std::string("conn-001"));
    QCOMPARE(decoded.properties().at("capacity"), std::string("10"));
    QCOMPARE(decoded.properties().at("latency"),  std::string("5ms"));
}

// ============================================================================
// public_api.proto
// ============================================================================

void tst_Phase1ProtoContracts::actionListRequest_roundtrip()
{
    cme::publicapi::v1::ActionListRequest req;
    req.mutable_provider()->set_provider_id("provider.actions");
    req.mutable_provider()->set_interface_iid("org.cme.IActionProvider/2.0");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.provider().provider_id(), std::string("provider.actions"));
    QCOMPARE(decoded.provider().interface_iid(), std::string("org.cme.IActionProvider/2.0"));
}

void tst_Phase1ProtoContracts::actionListResponse_roundtrip()
{
    cme::publicapi::v1::ActionListResponse resp;
    resp.mutable_status()->set_success(true);
    auto *action = resp.add_actions();
    action->set_action_id("action.add");
    action->set_title("Add Component");
    action->set_enabled(true);

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.actions_size(), 1);
    QCOMPARE(decoded.actions(0).action_id(), std::string("action.add"));
}

void tst_Phase1ProtoContracts::actionDescriptorRequest_roundtrip()
{
    cme::publicapi::v1::ActionDescriptorRequest req;
    req.mutable_provider()->set_provider_id("provider.actions");
    req.set_action_id("action.run");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.provider().provider_id(), std::string("provider.actions"));
    QCOMPARE(decoded.action_id(), std::string("action.run"));
}

void tst_Phase1ProtoContracts::actionDescriptorResponse_roundtrip()
{
    cme::publicapi::v1::ActionDescriptorResponse resp;
    resp.mutable_status()->set_success(true);
    auto *descriptor = resp.mutable_action_descriptor();
    descriptor->set_action_id("action.run");
    descriptor->set_title("Run");
    descriptor->set_hint("Runs current graph");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.action_descriptor().action_id(), std::string("action.run"));
    QCOMPARE(decoded.action_descriptor().hint(), std::string("Runs current graph"));
}

void tst_Phase1ProtoContracts::actionInvokeRequest_roundtrip()
{
    cme::publicapi::v1::ActionInvokeRequest req;
    req.mutable_provider()->set_provider_id("provider.actions");
    req.set_action_id("action.run");
    (*req.mutable_context()->mutable_values())["mode"].set_string_value("fast");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.action_id(), std::string("action.run"));
    QCOMPARE(decoded.context().values().at("mode").string_value(), std::string("fast"));
}

void tst_Phase1ProtoContracts::actionInvokeResponse_roundtrip()
{
    cme::publicapi::v1::ActionInvokeResponse resp;
    resp.mutable_status()->set_success(true);
    resp.mutable_command_request()->mutable_add_component()->set_type_id("logic/AndGate");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QVERIFY(decoded.command_request().has_add_component());
    QCOMPARE(decoded.command_request().add_component().type_id(), std::string("logic/AndGate"));
}

void tst_Phase1ProtoContracts::componentTypeIdsRequest_roundtrip()
{
    cme::publicapi::v1::ComponentTypeIdsRequest req;
    req.mutable_provider()->set_provider_id("provider.types");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.provider().provider_id(), std::string("provider.types"));
}

void tst_Phase1ProtoContracts::componentTypeIdsResponse_roundtrip()
{
    cme::publicapi::v1::ComponentTypeIdsResponse resp;
    resp.mutable_status()->set_success(true);
    resp.add_component_type_ids("logic/AndGate");
    resp.add_component_type_ids("logic/OrGate");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.component_type_ids_size(), 2);
    QCOMPARE(decoded.component_type_ids(1), std::string("logic/OrGate"));
}

void tst_Phase1ProtoContracts::componentTypeDescriptorRequest_roundtrip()
{
    cme::publicapi::v1::ComponentTypeDescriptorRequest req;
    req.mutable_provider()->set_provider_id("provider.types");
    req.set_component_type_id("logic/AndGate");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.component_type_id(), std::string("logic/AndGate"));
}

void tst_Phase1ProtoContracts::componentTypeDescriptorResponse_roundtrip()
{
    cme::publicapi::v1::ComponentTypeDescriptorResponse resp;
    resp.mutable_status()->set_success(true);
    auto *descriptor = resp.mutable_component_type_descriptor();
    descriptor->set_id("logic/AndGate");
    descriptor->set_title("AND");
    descriptor->set_default_width(120.0);

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.component_type_descriptor().id(), std::string("logic/AndGate"));
    QCOMPARE(decoded.component_type_descriptor().default_width(), 120.0);
}

void tst_Phase1ProtoContracts::defaultComponentPropertiesRequest_roundtrip()
{
    cme::publicapi::v1::DefaultComponentPropertiesRequest req;
    req.mutable_provider()->set_provider_id("provider.types");
    req.set_component_type_id("logic/AndGate");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.component_type_id(), std::string("logic/AndGate"));
}

void tst_Phase1ProtoContracts::defaultComponentPropertiesResponse_roundtrip()
{
    cme::publicapi::v1::DefaultComponentPropertiesResponse resp;
    resp.mutable_status()->set_success(true);
    (*resp.mutable_properties())["label"].set_string_value("A");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.properties().at("label").string_value(), std::string("A"));
}

void tst_Phase1ProtoContracts::canConnectRequest_roundtrip()
{
    cme::publicapi::v1::CanConnectRequest req;
    req.mutable_provider()->set_provider_id("provider.policy");
    req.set_source_type_id("start");
    req.set_target_type_id("process");
    req.mutable_context()->set_source_type_id("start");
    req.mutable_context()->set_target_type_id("process");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.source_type_id(), std::string("start"));
    QCOMPARE(decoded.context().target_type_id(), std::string("process"));
}

void tst_Phase1ProtoContracts::canConnectResponse_roundtrip()
{
    cme::publicapi::v1::CanConnectResponse resp;
    resp.mutable_status()->set_success(true);
    resp.set_allowed(true);
    resp.set_reason("");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QVERIFY(decoded.allowed());
    QVERIFY(decoded.reason().empty());
}

void tst_Phase1ProtoContracts::normalizeConnectionPropertiesRequest_roundtrip()
{
    cme::publicapi::v1::NormalizeConnectionPropertiesRequest req;
    req.mutable_provider()->set_provider_id("provider.policy");
    req.set_source_type_id("process");
    req.set_target_type_id("stop");
    (*req.mutable_raw_properties())["type"].set_string_value("flow");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.source_type_id(), std::string("process"));
    QCOMPARE(decoded.raw_properties().at("type").string_value(), std::string("flow"));
}

void tst_Phase1ProtoContracts::normalizeConnectionPropertiesResponse_roundtrip()
{
    cme::publicapi::v1::NormalizeConnectionPropertiesResponse resp;
    resp.mutable_status()->set_success(true);
    (*resp.mutable_normalized_properties())["type"].set_string_value("flow");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.normalized_properties().at("type").string_value(), std::string("flow"));
}

void tst_Phase1ProtoContracts::supportedComponentTypesRequest_roundtrip()
{
    cme::publicapi::v1::SupportedComponentTypesRequest req;
    req.mutable_provider()->set_provider_id("provider.exec");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.provider().provider_id(), std::string("provider.exec"));
}

void tst_Phase1ProtoContracts::supportedComponentTypesResponse_roundtrip()
{
    cme::publicapi::v1::SupportedComponentTypesResponse resp;
    resp.mutable_status()->set_success(true);
    resp.add_component_types("logic/AndGate");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.component_types(0), std::string("logic/AndGate"));
}

void tst_Phase1ProtoContracts::executeComponentRequest_roundtrip()
{
    cme::publicapi::v1::ExecuteComponentRequest req;
    req.mutable_provider()->set_provider_id("provider.exec");
    req.set_component_type("logic/AndGate");
    req.set_component_id("comp-001");
    req.mutable_component_snapshot()->set_id("comp-001");
    (*req.mutable_input_state())["in0"].set_string_value("1");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.component_type(), std::string("logic/AndGate"));
    QCOMPARE(decoded.component_snapshot().id(), std::string("comp-001"));
    QCOMPARE(decoded.input_state().at("in0").string_value(), std::string("1"));
}

void tst_Phase1ProtoContracts::executeComponentResponse_roundtrip()
{
    cme::publicapi::v1::ExecuteComponentResponse resp;
    resp.mutable_status()->set_success(true);
    (*resp.mutable_output_state())["out0"].set_string_value("1");
    (*resp.mutable_trace())["step"].set_string_value("done");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.output_state().at("out0").string_value(), std::string("1"));
    QCOMPARE(decoded.trace().at("step").string_value(), std::string("done"));
}

void tst_Phase1ProtoContracts::schemaTargetsRequest_roundtrip()
{
    cme::publicapi::v1::SchemaTargetsRequest req;
    req.mutable_provider()->set_provider_id("provider.schema");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.provider().provider_id(), std::string("provider.schema"));
}

void tst_Phase1ProtoContracts::schemaTargetsResponse_roundtrip()
{
    cme::publicapi::v1::SchemaTargetsResponse resp;
    resp.mutable_status()->set_success(true);
    resp.add_target_ids("logic/AndGate");

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.target_ids(0), std::string("logic/AndGate"));
}

void tst_Phase1ProtoContracts::propertySchemaRequest_roundtrip()
{
    cme::publicapi::v1::PropertySchemaRequest req;
    req.mutable_provider()->set_provider_id("provider.schema");
    req.set_target_id("logic/AndGate");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.target_id(), std::string("logic/AndGate"));
}

void tst_Phase1ProtoContracts::propertySchemaResponse_roundtrip()
{
    cme::publicapi::v1::PropertySchemaResponse resp;
    resp.mutable_status()->set_success(true);
    auto *entry = resp.add_entries();
    entry->set_key("inputs");
    entry->set_type("int");
    entry->set_required(true);

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.entries_size(), 1);
    QCOMPARE(decoded.entries(0).key(), std::string("inputs"));
}

void tst_Phase1ProtoContracts::validateGraphRequest_roundtrip()
{
    cme::publicapi::v1::ValidateGraphRequest req;
    req.mutable_provider()->set_provider_id("provider.validation");
    req.mutable_graph_snapshot()->set_graph_id("graph-42");

    const auto decoded = roundtrip(req);
    QCOMPARE(decoded.provider().provider_id(), std::string("provider.validation"));
    QCOMPARE(decoded.graph_snapshot().graph_id(), std::string("graph-42"));
}

void tst_Phase1ProtoContracts::validateGraphResponse_roundtrip()
{
    cme::publicapi::v1::ValidateGraphResponse resp;
    resp.mutable_status()->set_success(true);
    resp.mutable_result()->set_is_valid(true);

    const auto decoded = roundtrip(resp);
    QVERIFY(decoded.status().success());
    QVERIFY(decoded.result().is_valid());
}

void tst_Phase1ProtoContracts::executionSnapshotEnvelope_roundtrip()
{
    cme::publicapi::v1::ExecutionSnapshotEnvelope env;
    env.mutable_status()->set_success(true);
    env.mutable_snapshot()->set_session_id("session-001");

    const auto decoded = roundtrip(env);
    QVERIFY(decoded.status().success());
    QCOMPARE(decoded.snapshot().session_id(), std::string("session-001"));
}

// ============================================================================
// provider_templates.proto
// ============================================================================

void tst_Phase1ProtoContracts::propertySchemaFieldTemplate_roundtrip()
{
    cme::templates::v1::PropertySchemaFieldTemplate field;
    field.set_key("addValue");
    field.set_type("number");
    field.set_title("Add Value");
    field.set_required(true);
    field.mutable_default_value()->set_number_value(9);
    field.set_editor("spinbox");
    field.set_section("Behavior");
    field.set_order(21);
    field.set_hint("Number added to running total");
    field.add_options()->set_string_value("9");
    (*field.mutable_validation())["min"].set_number_value(-1000000);
    (*field.mutable_visible_when())["enabled"].set_bool_value(true);

    const auto decoded = roundtrip(field);
    QCOMPARE(decoded.key(), std::string("addValue"));
    QCOMPARE(decoded.type(), std::string("number"));
    QCOMPARE(decoded.required(), true);
    QCOMPARE(decoded.default_value().number_value(), 9.0);
    QCOMPARE(decoded.validation().at("min").number_value(), -1000000.0);
    QCOMPARE(decoded.visible_when().at("enabled").bool_value(), true);
}

void tst_Phase1ProtoContracts::propertySchemaTemplateBundle_roundtrip()
{
    cme::templates::v1::PropertySchemaTemplateBundle bundle;
    bundle.set_provider_id("customize.workflow.propertySchema");
    bundle.set_schema_version("1.0.0");

    auto *target = bundle.add_targets();
    target->set_target_id("component/process");
    auto *field = target->add_entries();
    field->set_key("title");
    field->set_type("string");
    field->set_title("Title");
    field->set_required(true);
    field->set_editor("textfield");

    const auto decoded = roundtrip(bundle);
    QCOMPARE(decoded.provider_id(), std::string("customize.workflow.propertySchema"));
    QCOMPARE(decoded.schema_version(), std::string("1.0.0"));
    QCOMPARE(decoded.targets_size(), 1);
    QCOMPARE(decoded.targets(0).target_id(), std::string("component/process"));
    QCOMPARE(decoded.targets(0).entries_size(), 1);
    QCOMPARE(decoded.targets(0).entries(0).key(), std::string("title"));
}

void tst_Phase1ProtoContracts::componentTypeTemplateBundle_roundtrip()
{
    cme::templates::v1::ComponentTypeTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.componentTypes");
    bundle.set_schema_version("1.0.0");

    auto *type = bundle.add_component_types();
    type->set_id("start");
    type->set_title("Start");
    type->set_category("control");
    type->set_default_width(92.0);
    type->set_default_height(92.0);
    type->set_default_color("#66bb6a");
    type->set_allow_incoming(false);
    type->set_allow_outgoing(true);

    auto *defaults = bundle.add_defaults();
    defaults->set_component_type_id("start");
    (*defaults->mutable_properties())["inputNumber"].set_number_value(0);

    const auto decoded = roundtrip(bundle);
    QCOMPARE(decoded.provider_id(), std::string("sample.workflow.componentTypes"));
    QCOMPARE(decoded.component_types_size(), 1);
    QCOMPARE(decoded.component_types(0).id(), std::string("start"));
    QCOMPARE(decoded.component_types(0).allow_outgoing(), true);
    QCOMPARE(decoded.defaults_size(), 1);
    QCOMPARE(decoded.defaults(0).properties().at("inputNumber").number_value(), 0.0);
}

void tst_Phase1ProtoContracts::connectionPolicyTemplateBundle_roundtrip()
{
    cme::templates::v1::ConnectionPolicyTemplateBundle bundle;
    bundle.set_provider_id("sample.workflow.connectionPolicy");
    bundle.set_schema_version("1.0.0");
    bundle.set_default_allowed(false);
    bundle.set_default_reason("Unknown");
    bundle.set_normalized_type_key("type");
    bundle.set_normalized_type_value("flow");
    bundle.mutable_transport()->set_payload_schema_id("workflow.connection.payload");
    bundle.mutable_transport()->set_payload_type("workflow/flow");
    bundle.mutable_transport()->set_transport_mode("broadcast");
    bundle.mutable_transport()->set_merge_hint("preserve-per-edge");

    auto *rule = bundle.add_rules();
    rule->set_source_type_id("start");
    rule->set_target_type_id("process");
    rule->set_allowed(true);
    rule->set_reason("");

    auto *denyRule = bundle.add_rules();
    denyRule->set_target_type_id("condition");
    denyRule->set_allowed(false);
    denyRule->set_reason("Condition component accepts only one incoming connection.");
    denyRule->set_min_target_incoming(1);

    const auto decoded = roundtrip(bundle);
    QCOMPARE(decoded.provider_id(), std::string("sample.workflow.connectionPolicy"));
    QCOMPARE(decoded.rules_size(), 2);
    QCOMPARE(decoded.rules(0).source_type_id(), std::string("start"));
    QCOMPARE(decoded.rules(1).has_min_target_incoming(), true);
    QCOMPARE(decoded.rules(1).min_target_incoming(), 1);
    QCOMPARE(decoded.normalized_type_value(), std::string("flow"));
    QVERIFY(decoded.has_transport());
    QCOMPARE(decoded.transport().payload_schema_id(), std::string("workflow.connection.payload"));
    QCOMPARE(decoded.transport().payload_type(), std::string("workflow/flow"));
    QCOMPARE(decoded.transport().transport_mode(), std::string("broadcast"));
    QCOMPARE(decoded.transport().merge_hint(), std::string("preserve-per-edge"));
}

QTEST_MAIN(tst_Phase1ProtoContracts)
#include "tst_Phase1ProtoContracts.moc"
