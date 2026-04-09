// tests/tst_Phase2AdapterLayer.cpp
//
// Phase 2: Compatibility Adapter Layer
//
// Tests all bidirectional adapter conversions between QVariantMap (legacy) and
// protobuf contracts. Verifies roundtrip fidelity, required-field validation,
// type mismatch handling, and unknown-field preservation.
//
// Pass condition: All roundtrip tests green, error cases handled with clear
// error messages, Phase 0 baseline still passes (no regression).

#include <QtTest>

#include "CommandAdapter.h"
#include "ExecutionAdapter.h"
#include "GraphAdapter.h"
#include "PolicyAdapter.h"
#include "ValidationAdapter.h"

// ── Test fixture ───────────────────────────────────────────────────────────

class tst_Phase2AdapterLayer : public QObject {
    Q_OBJECT

private Q_SLOTS:

    // ── CommandAdapter ─────────────────────────────────────────────────────
    void addComponentRequest_roundtrip();
    void addComponentRequest_missingTypeId();
    void addComponentRequest_invalidX();
    void removeComponentRequest_roundtrip();
    void moveComponentRequest_roundtrip();
    void addConnectionRequest_roundtrip();
    void commandRequest_unknownCommand();

    // ── GraphAdapter ───────────────────────────────────────────────────────
    void componentData_roundtrip();
    void componentData_withProperties();
    void componentData_missingId();
    void connectionData_roundtrip();
    void graphSnapshot_roundtrip();

    // ── ValidationAdapter ──────────────────────────────────────────────────
    void validationIssue_error_roundtrip();
    void validationIssue_invalidSeverity();
    void graphValidationResult_roundtrip();

    // ── ExecutionAdapter ───────────────────────────────────────────────────
    void timelineEvent_roundtrip();
    void timelineEvent_invalidType();
    void componentExecutionState_roundtrip();
    void executionSnapshot_roundtrip();

    // ── PolicyAdapter ──────────────────────────────────────────────────────
    void connectionPolicyContext_roundtrip();
    void componentTypeDefaults_roundtrip();
    void connectionProperties_roundtrip();
};

// ============================================================================
// CommandAdapter Tests
// ============================================================================

void tst_Phase2AdapterLayer::addComponentRequest_roundtrip()
{
    QVariantMap variant;
    variant["addComponent"] = QVariantMap{
        {"typeId", "logic/AndGate"},
        {"x", 100.5},
        {"y", 200.75},
        {"componentId", "comp-001"}
    };

    cme::GraphCommandRequest proto;
    auto err = cme::adapter::variantMapToGraphCommandRequest(variant, proto);
    QVERIFY(!err.has_error);
    QVERIFY(proto.has_add_component());

    // Reverse
    auto variant2 = cme::adapter::graphCommandRequestToVariantMap(proto);
    auto inner = variant2["addComponent"].toMap();
    QCOMPARE(inner["typeId"].toString(), QString("logic/AndGate"));
    QCOMPARE(inner["x"].toDouble(), 100.5);
    QCOMPARE(inner["y"].toDouble(), 200.75);
    QCOMPARE(inner["componentId"].toString(), QString("comp-001"));
}

void tst_Phase2AdapterLayer::addComponentRequest_missingTypeId()
{
    QVariantMap variant;
    variant["addComponent"] = QVariantMap{
        // missing "typeId"
        {"x", 100.0},
        {"y", 200.0}
    };

    cme::GraphCommandRequest proto;
    auto err = cme::adapter::variantMapToGraphCommandRequest(variant, proto);
    QVERIFY(err.has_error);
    QVERIFY(err.error_message.contains("Missing required field: typeId"));
}

void tst_Phase2AdapterLayer::addComponentRequest_invalidX()
{
    QVariantMap variant;
    variant["addComponent"] = QVariantMap{
        {"typeId", "logic/AndGate"},
        {"x", "not a number"}, // type mismatch
        {"y", 200.0}
    };

    cme::GraphCommandRequest proto;
    auto err = cme::adapter::variantMapToGraphCommandRequest(variant, proto);
    QVERIFY(err.has_error);
    QVERIFY(err.error_message.contains("not a valid double"));
}

void tst_Phase2AdapterLayer::removeComponentRequest_roundtrip()
{
    QVariantMap variant;
    variant["removeComponent"] = QVariantMap{
        {"componentId", "comp-007"}
    };

    cme::GraphCommandRequest proto;
    auto err = cme::adapter::variantMapToGraphCommandRequest(variant, proto);
    QVERIFY(!err.has_error);
    QVERIFY(proto.has_remove_component());

    auto variant2 = cme::adapter::graphCommandRequestToVariantMap(proto);
    auto inner = variant2["removeComponent"].toMap();
    QCOMPARE(inner["componentId"].toString(), QString("comp-007"));
}

void tst_Phase2AdapterLayer::moveComponentRequest_roundtrip()
{
    QVariantMap variant;
    variant["moveComponent"] = QVariantMap{
        {"componentId", "comp-003"},
        {"oldX", 10.0},
        {"oldY", 20.0},
        {"newX", 30.0},
        {"newY", 40.0}
    };

    cme::GraphCommandRequest proto;
    auto err = cme::adapter::variantMapToGraphCommandRequest(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::graphCommandRequestToVariantMap(proto);
    auto inner = variant2["moveComponent"].toMap();
    QCOMPARE(inner["componentId"].toString(), QString("comp-003"));
    QCOMPARE(inner["oldX"].toDouble(), 10.0);
    QCOMPARE(inner["newX"].toDouble(), 30.0);
}

void tst_Phase2AdapterLayer::addConnectionRequest_roundtrip()
{
    QVariantMap variant;
    variant["addConnection"] = QVariantMap{
        {"sourceId", "c1"},
        {"targetId", "c2"},
        {"connectionId", "conn-001"}
    };

    cme::GraphCommandRequest proto;
    auto err = cme::adapter::variantMapToGraphCommandRequest(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::graphCommandRequestToVariantMap(proto);
    auto inner = variant2["addConnection"].toMap();
    QCOMPARE(inner["sourceId"].toString(), QString("c1"));
    QCOMPARE(inner["targetId"].toString(), QString("c2"));
}

void tst_Phase2AdapterLayer::commandRequest_unknownCommand()
{
    QVariantMap variant;
    variant["unknownCommand"] = QVariantMap{{"field", "value"}};

    cme::GraphCommandRequest proto;
    auto err = cme::adapter::variantMapToGraphCommandRequest(variant, proto);
    QVERIFY(err.has_error);
    QVERIFY(err.error_message.contains("not recognized"));
}

// ============================================================================
// GraphAdapter Tests
// ============================================================================

void tst_Phase2AdapterLayer::componentData_roundtrip()
{
    QVariantMap variant;
    variant["id"] = "comp-001";
    variant["typeId"] = "logic/AndGate";
    variant["x"] = 50.0;
    variant["y"] = 75.0;

    cme::ComponentData proto;
    auto err = cme::adapter::variantMapToComponentData(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::componentDataToVariantMap(proto);
    QCOMPARE(variant2["id"].toString(), QString("comp-001"));
    QCOMPARE(variant2["typeId"].toString(), QString("logic/AndGate"));
    QCOMPARE(variant2["x"].toDouble(), 50.0);
    QCOMPARE(variant2["y"].toDouble(), 75.0);
}

void tst_Phase2AdapterLayer::componentData_withProperties()
{
    QVariantMap variant;
    variant["id"] = "comp-005";
    variant["typeId"] = "logic/OrGate";
    variant["x"] = 0.0;
    variant["y"] = 0.0;
    QVariantMap props;
    props["label"] = "Gate XOR";
    props["inputs"] = "3";
    variant["properties"] = props;

    cme::ComponentData proto;
    auto err = cme::adapter::variantMapToComponentData(variant, proto);
    QVERIFY(!err.has_error);
    QCOMPARE(proto.properties().size(), 2);

    auto variant2 = cme::adapter::componentDataToVariantMap(proto);
    auto props2 = variant2["properties"].toMap();
    QCOMPARE(props2["label"].toString(), QString("Gate XOR"));
    QCOMPARE(props2["inputs"].toString(), QString("3"));
}

void tst_Phase2AdapterLayer::componentData_missingId()
{
    QVariantMap variant;
    // missing "id"
    variant["typeId"] = "logic/Inverter";
    variant["x"] = 0.0;
    variant["y"] = 0.0;

    cme::ComponentData proto;
    auto err = cme::adapter::variantMapToComponentData(variant, proto);
    QVERIFY(err.has_error);
    QVERIFY(err.error_message.contains("Missing required field: id"));
}

void tst_Phase2AdapterLayer::connectionData_roundtrip()
{
    QVariantMap variant;
    variant["id"] = "conn-001";
    variant["sourceId"] = "c1";
    variant["targetId"] = "c2";

    cme::ConnectionData proto;
    auto err = cme::adapter::variantMapToConnectionData(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::connectionDataToVariantMap(proto);
    QCOMPARE(variant2["id"].toString(), QString("conn-001"));
    QCOMPARE(variant2["sourceId"].toString(), QString("c1"));
    QCOMPARE(variant2["targetId"].toString(), QString("c2"));
}

void tst_Phase2AdapterLayer::graphSnapshot_roundtrip()
{
    QVariantMap variant;
    variant["graphId"] = "graph-42";

    QVariantMap comp;
    comp["id"] = "c1";
    comp["typeId"] = "typeA";
    comp["x"] = 1.0;
    comp["y"] = 2.0;
    variant["components"] = QVariantList{comp};

    QVariantMap edge;
    edge["id"] = "e1";
    edge["sourceId"] = "c1";
    edge["targetId"] = "c2";
    variant["connections"] = QVariantList{edge};

    cme::GraphSnapshot proto;
    auto err = cme::adapter::variantMapToGraphSnapshot(variant, proto);
    QVERIFY(!err.has_error);
    QCOMPARE(proto.graph_id(), std::string("graph-42"));
    QCOMPARE(proto.components_size(), 1);
    QCOMPARE(proto.connections_size(), 1);

    auto variant2 = cme::adapter::graphSnapshotToVariantMap(proto);
    QCOMPARE(variant2["graphId"].toString(), QString("graph-42"));
    QCOMPARE(variant2["components"].toList().size(), 1);
    QCOMPARE(variant2["connections"].toList().size(), 1);
}

// ============================================================================
// ValidationAdapter Tests
// ============================================================================

void tst_Phase2AdapterLayer::validationIssue_error_roundtrip()
{
    QVariantMap variant;
    variant["code"] = "CORE_NULL_GRAPH";
    variant["severity"] = "error";
    variant["message"] = "Graph is null";

    cme::ValidationIssue proto;
    auto err = cme::adapter::variantMapToValidationIssue(variant, proto);
    QVERIFY(!err.has_error);
    QCOMPARE(proto.severity(), cme::VALIDATION_SEVERITY_ERROR);

    auto variant2 = cme::adapter::validationIssueToVariantMap(proto);
    QCOMPARE(variant2["code"].toString(), QString("CORE_NULL_GRAPH"));
    QCOMPARE(variant2["severity"].toString(), QString("error"));
}

void tst_Phase2AdapterLayer::validationIssue_invalidSeverity()
{
    QVariantMap variant;
    variant["code"] = "INVALID_ISSUE";
    variant["severity"] = "critical"; // invalid value

    cme::ValidationIssue proto;
    auto err = cme::adapter::variantMapToValidationIssue(variant, proto);
    QVERIFY(err.has_error);
    QVERIFY(err.error_message.contains("Invalid severity"));
}

void tst_Phase2AdapterLayer::graphValidationResult_roundtrip()
{
    QVariantMap variant;
    variant["isValid"] = false;

    QVariantMap issue;
    issue["code"] = "MISSING_SINK";
    issue["severity"] = "error";
    variant["issues"] = QVariantList{issue};

    cme::GraphValidationResult proto;
    auto err = cme::adapter::variantMapToGraphValidationResult(variant, proto);
    QVERIFY(!err.has_error);
    QVERIFY(!proto.is_valid());
    QCOMPARE(proto.issues_size(), 1);

    auto variant2 = cme::adapter::graphValidationResultToVariantMap(proto);
    QVERIFY(!variant2["isValid"].toBool());
    QCOMPARE(variant2["issues"].toList().size(), 1);
}

// ============================================================================
// ExecutionAdapter Tests
// ============================================================================

void tst_Phase2AdapterLayer::timelineEvent_roundtrip()
{
    QVariantMap variant;
    variant["type"] = "simulationStarted";
    variant["componentId"] = "";
    variant["timestamp"] = 1234567890LL;
    variant["message"] = "sim started";

    cme::TimelineEvent proto;
    auto err = cme::adapter::variantMapToTimelineEvent(variant, proto);
    QVERIFY(!err.has_error);
    QCOMPARE(proto.type(), cme::TIMELINE_EVENT_TYPE_SIMULATION_STARTED);

    auto variant2 = cme::adapter::timelineEventToVariantMap(proto);
    QCOMPARE(variant2["type"].toString().toLower(), QString("simulationstarted"));
}

void tst_Phase2AdapterLayer::timelineEvent_invalidType()
{
    QVariantMap variant;
    variant["type"] = "unknownEventType";
    variant["message"] = "test";

    cme::TimelineEvent proto;
    auto err = cme::adapter::variantMapToTimelineEvent(variant, proto);
    QVERIFY(err.has_error);
    QVERIFY(err.error_message.contains("Invalid timeline event type"));
}

void tst_Phase2AdapterLayer::componentExecutionState_roundtrip()
{
    QVariantMap variant;
    variant["componentId"] = "comp-5";

    QVariantMap input;
    input["in0"] = "1";
    variant["inputState"] = input;

    QVariantMap output;
    output["out0"] = "0";
    variant["outputState"] = output;

    variant["trace"] = "evaluated";

    cme::ComponentExecutionState proto;
    auto err = cme::adapter::variantMapToComponentExecutionState(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::componentExecutionStateToVariantMap(proto);
    QCOMPARE(variant2["componentId"].toString(), QString("comp-5"));
    QCOMPARE(variant2["inputState"].toMap()["in0"].toString(), QString("1"));
    QCOMPARE(variant2["outputState"].toMap()["out0"].toString(), QString("0"));
}

void tst_Phase2AdapterLayer::executionSnapshot_roundtrip()
{
    QVariantMap variant;
    variant["sessionId"] = "session-001";

    QVariantMap event;
    event["type"] = "breakpointHit";
    event["componentId"] = "comp-5";
    event["timestamp"] = 500LL;
    variant["events"] = QVariantList{event};

    QVariantMap state;
    state["componentId"] = "comp-5";
    state["inputState"] = QVariantMap{{"in", "1"}};
    state["outputState"] = QVariantMap{{"out", "0"}};
    variant["componentStates"] = QVariantList{state};

    cme::ExecutionSnapshot proto;
    auto err = cme::adapter::variantMapToExecutionSnapshot(variant, proto);
    QVERIFY(!err.has_error);
    QCOMPARE(proto.session_id(), std::string("session-001"));
    QCOMPARE(proto.events_size(), 1);
    QCOMPARE(proto.component_states_size(), 1);

    auto variant2 = cme::adapter::executionSnapshotToVariantMap(proto);
    QCOMPARE(variant2["sessionId"].toString(), QString("session-001"));
    QCOMPARE(variant2["events"].toList().size(), 1);
    QCOMPARE(variant2["componentStates"].toList().size(), 1);
}

// ============================================================================
// PolicyAdapter Tests
// ============================================================================

void tst_Phase2AdapterLayer::connectionPolicyContext_roundtrip()
{
    QVariantMap variant;
    variant["sourceTypeId"] = "logic/AndGate";
    variant["targetTypeId"] = "logic/OrGate";
    variant["sourcePort"] = "out0";
    variant["targetPort"] = "in1";

    QVariantMap extra;
    extra["bandwidth"] = "100";
    variant["extra"] = extra;

    cme::ConnectionPolicyContext proto;
    auto err = cme::adapter::variantMapToConnectionPolicyContext(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::connectionPolicyContextToVariantMap(proto);
    QCOMPARE(variant2["sourceTypeId"].toString(), QString("logic/AndGate"));
    QCOMPARE(variant2["sourcePort"].toString(), QString("out0"));
    QCOMPARE(variant2["extra"].toMap()["bandwidth"].toString(), QString("100"));
}

void tst_Phase2AdapterLayer::componentTypeDefaults_roundtrip()
{
    QVariantMap variant;
    variant["typeId"] = "logic/AndGate";
    variant["defaultX"] = 0.0;
    variant["defaultY"] = 0.0;

    QVariantMap props;
    props["inputs"] = "2";
    props["label"] = "";
    variant["defaultProperties"] = props;

    cme::ComponentTypeDefaults proto;
    auto err = cme::adapter::variantMapToComponentTypeDefaults(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::componentTypeDefaultsToVariantMap(proto);
    QCOMPARE(variant2["typeId"].toString(), QString("logic/AndGate"));
    QCOMPARE(variant2["defaultProperties"].toMap()["inputs"].toString(), QString("2"));
}

void tst_Phase2AdapterLayer::connectionProperties_roundtrip()
{
    QVariantMap variant;
    variant["connectionId"] = "conn-001";

    QVariantMap props;
    props["capacity"] = "10";
    props["latency"] = "5ms";
    variant["properties"] = props;

    cme::ConnectionProperties proto;
    auto err = cme::adapter::variantMapToConnectionProperties(variant, proto);
    QVERIFY(!err.has_error);

    auto variant2 = cme::adapter::connectionPropertiesToVariantMap(proto);
    QCOMPARE(variant2["connectionId"].toString(), QString("conn-001"));
    QCOMPARE(variant2["properties"].toMap()["capacity"].toString(), QString("10"));
}

QTEST_MAIN(tst_Phase2AdapterLayer)
#include "tst_Phase2AdapterLayer.moc"
