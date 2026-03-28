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

QTEST_MAIN(tst_Phase1ProtoContracts)
#include "tst_Phase1ProtoContracts.moc"
