// tests/tst_Phase4PolicyMigration.cpp
//
// Phase 4: GraphEditorController Policy Migration
//
// Verifies that the typed connection policy context implementation:
//   1. Adapters convert typed↔map while preserving data
//   2. Policy decisions are identical to baseline
//   3. Signal behavior is unchanged
//   4. Drag-connect with side values preserves context

#include <QtTest>
#include <QSignalSpy>
#include <QVariantMap>

#include "services/GraphEditorController.h"
#include "models/GraphModel.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "commands/UndoStack.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/IConnectionPolicyProvider.h"
#include "adapters/PolicyAdapter.h"

class MockConnectionPolicyProviderPhase4 : public IConnectionPolicyProvider
{
public:
    QString providerId() const override { return QStringLiteral("mockPhase4"); }

    bool canConnect(const cme::ConnectionPolicyContext &context,
                    QString *reason) const override
    {
        const QString srcType = QString::fromStdString(context.source_type_id());
        const QString tgtType = QString::fromStdString(context.target_type_id());
        const QVariantMap contextMap = cme::adapter::connectionPolicyContextToVariantMap(context);

        // Reject if source and target are the same type and connection count > 2
        if (srcType == tgtType && contextMap.value("graphConnectionCount").toInt() > 2) {
            if (reason)
                *reason = QStringLiteral("Max connections reached");
            return false;
        }
        return true;
    }

    QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &,
                                              const QVariantMap &rawProps) const override
    {
        QVariantMap props = rawProps;
        if (!props.contains("label"))
            props["label"] = QStringLiteral("normalized");
        return props;
    }
};

class TestPhase4PolicyMigration : public QObject
{
    Q_OBJECT

private slots:
    // ── Test: Adapter conversions ──────────────────────────────────────

    void test_adapter_contextProtoToVariantMap()
    {
        cme::ConnectionPolicyContext proto;
        proto.set_source_component_id("comp1");
        proto.set_target_component_id("comp2");
        proto.set_source_type_id("TypeA");
        proto.set_target_type_id("TypeB");
        proto.set_source_outgoing_count(2);
        proto.set_source_incoming_count(1);
        proto.set_target_outgoing_count(3);
        proto.set_target_incoming_count(0);
        proto.set_graph_connection_count(5);

        QVariantMap map = cme::adapter::connectionPolicyContextToVariantMap(proto);

        QCOMPARE(map.value("sourceComponentId").toString(), QString("comp1"));
        QCOMPARE(map.value("targetComponentId").toString(), QString("comp2"));
        QCOMPARE(map.value("sourceTypeId").toString(), QString("TypeA"));
        QCOMPARE(map.value("targetTypeId").toString(), QString("TypeB"));
        QCOMPARE(map.value("sourceOutgoingCount").toInt(), 2);
        QCOMPARE(map.value("sourceIncomingCount").toInt(), 1);
        QCOMPARE(map.value("targetOutgoingCount").toInt(), 3);
        QCOMPARE(map.value("targetIncomingCount").toInt(), 0);
        QCOMPARE(map.value("graphConnectionCount").toInt(), 5);
    }

    void test_adapter_contextVariantMapToProto()
    {
        QVariantMap map;
        map["sourceComponentId"] = "comp1";
        map["targetComponentId"] = "comp2";
        map["sourceTypeId"] = "TypeA";
        map["targetTypeId"] = "TypeB";
        map["sourceOutgoingCount"] = 2;
        map["sourceIncomingCount"] = 1;
        map["targetOutgoingCount"] = 3;
        map["targetIncomingCount"] = 0;
        map["graphConnectionCount"] = 5;

        cme::ConnectionPolicyContext proto;
        cme::adapter::variantMapToConnectionPolicyContext(map, proto);

        QCOMPARE(QString::fromStdString(proto.source_component_id()), QString("comp1"));
        QCOMPARE(QString::fromStdString(proto.target_component_id()), QString("comp2"));
        QCOMPARE(proto.source_outgoing_count(), 2);
        QCOMPARE(proto.source_incoming_count(), 1);
        QCOMPARE(proto.target_outgoing_count(), 3);
        QCOMPARE(proto.target_incoming_count(), 0);
        QCOMPARE(proto.graph_connection_count(), 5);
    }

    void test_adapter_roundtrip()
    {
        // Create original proto
        cme::ConnectionPolicyContext original;
        original.set_source_component_id("comp1");
        original.set_target_component_id("comp2");
        original.set_source_type_id("TypeA");
        original.set_source_outgoing_count(5);
        original.set_graph_connection_count(10);

        // Convert to map and back
        QVariantMap map = cme::adapter::connectionPolicyContextToVariantMap(original);
        cme::ConnectionPolicyContext reconstructed;
        cme::adapter::variantMapToConnectionPolicyContext(map, reconstructed);

        // Verify all fields match
        QCOMPARE(QString::fromStdString(reconstructed.source_component_id()),
                 QString::fromStdString(original.source_component_id()));
        QCOMPARE(QString::fromStdString(reconstructed.target_component_id()),
                 QString::fromStdString(original.target_component_id()));
        QCOMPARE(reconstructed.source_outgoing_count(), original.source_outgoing_count());
        QCOMPARE(reconstructed.graph_connection_count(), original.graph_connection_count());
    }

    // ── Test: Connection creation with policy ──────────────────────────

    void test_connectComponents_policyApproved()
    {
        ExtensionContractRegistry reg(ExtensionApiVersion{1, 0, 0});
        MockConnectionPolicyProviderPhase4 provider;
        reg.registerConnectionPolicyProvider(&provider);

        TypeRegistry typeRegistry;
        typeRegistry.rebuildFromRegistry(reg);

        GraphModel graph;
        UndoStack undoStack;
        GraphEditorController controller;

        controller.setGraph(&graph);
        controller.setUndoStack(&undoStack);
        controller.setTypeRegistry(&typeRegistry);

        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("TypeA"),
                                         0, 0, QStringLiteral("#fff"), QStringLiteral("TypeA"));
        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("TypeB"),
                                         100, 100, QStringLiteral("#aaa"), QStringLiteral("TypeB"));
        graph.addComponent(comp1);
        graph.addComponent(comp2);

        QSignalSpy spy(&controller, &GraphEditorController::connectionCreated);

        QString connId = controller.connectComponents(QStringLiteral("comp1"),
                                                      QStringLiteral("comp2"));

        QVERIFY(!connId.isEmpty());
        QCOMPARE(spy.count(), 1);
        QCOMPARE(graph.connectionCount(), 1);
    }

    void test_connectComponents_policyRejected()
    {
        ExtensionContractRegistry reg(ExtensionApiVersion{1, 0, 0});
        MockConnectionPolicyProviderPhase4 provider;
        reg.registerConnectionPolicyProvider(&provider);

        TypeRegistry typeRegistry;
        typeRegistry.rebuildFromRegistry(reg);

        GraphModel graph;
        UndoStack undoStack;
        GraphEditorController controller;

        controller.setGraph(&graph);
        controller.setUndoStack(&undoStack);
        controller.setTypeRegistry(&typeRegistry);

        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("TypeA"),
                                         0, 0, QStringLiteral("#fff"), QStringLiteral("TypeA"));
        graph.addComponent(comp1);

        // Add 3 connections ahead (will exceed mock provider's limit of 2)
        for (int i = 0; i < 3; ++i) {
            auto *comp = new ComponentModel(QStringLiteral("dummy%1").arg(i),
                                           QStringLiteral("TypeB"),
                                           100 * (i + 1), 100, QStringLiteral("#aaa"),
                                           QStringLiteral("TypeB"));
            graph.addComponent(comp);

            auto *conn = new ConnectionModel(
                QStringLiteral("conn%1").arg(i),
                QStringLiteral("comp1"),
                QStringLiteral("dummy%1").arg(i),
                QStringLiteral(""));
            graph.addConnection(conn);
        }

        QSignalSpy rejectedSpy(&controller, &GraphEditorController::connectionRejected);

        // Try to connect comp1 (TypeA) to another TypeA component
        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("TypeA"),
                                         300, 100, QStringLiteral("#fff"), QStringLiteral("TypeA"));
        graph.addComponent(comp2);

        QString connId = controller.connectComponents(QStringLiteral("comp1"),
                                                      QStringLiteral("comp2"));

        QVERIFY(connId.isEmpty());
        QCOMPARE(rejectedSpy.count(), 1);
        QCOMPARE(controller.lastConnectionRejectionReason(),
                 QStringLiteral("Max connections reached"));
    }

    void test_connectComponentsFromDrag_withSideValues()
    {
        ExtensionContractRegistry reg(ExtensionApiVersion{1, 0, 0});
        MockConnectionPolicyProviderPhase4 provider;
        reg.registerConnectionPolicyProvider(&provider);

        TypeRegistry typeRegistry;
        typeRegistry.rebuildFromRegistry(reg);

        GraphModel graph;
        UndoStack undoStack;
        GraphEditorController controller;

        controller.setGraph(&graph);
        controller.setUndoStack(&undoStack);
        controller.setTypeRegistry(&typeRegistry);

        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("TypeA"),
                                         0, 0, QStringLiteral("#fff"), QStringLiteral("TypeA"));
        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("TypeB"),
                                         100, 100, QStringLiteral("#aaa"), QStringLiteral("TypeB"));
        graph.addComponent(comp1);
        graph.addComponent(comp2);

        QSignalSpy spy(&controller, &GraphEditorController::connectionCreated);

        QString connId = controller.connectComponentsFromDrag(
            QStringLiteral("comp1"),
            QStringLiteral("comp2"),
            0,  // sourceSide
            1,  // targetSide
            QStringLiteral("custom_conn"),
            QStringLiteral("fallback_label"));

        QCOMPARE(connId, QStringLiteral("custom_conn"));
        QCOMPARE(spy.count(), 1);

        ConnectionModel *conn = graph.connectionById(QStringLiteral("custom_conn"));
        QVERIFY(conn);
        QCOMPARE(conn->label(), QStringLiteral("normalized")); // from mock provider
    }

    void test_connectComponentsFromDrag_fallbackLabel()
    {
        ExtensionContractRegistry reg(ExtensionApiVersion{1, 0, 0});
        MockConnectionPolicyProviderPhase4 provider;
        reg.registerConnectionPolicyProvider(&provider);

        TypeRegistry typeRegistry;
        typeRegistry.rebuildFromRegistry(reg);

        GraphModel graph;
        UndoStack undoStack;
        GraphEditorController controller;

        controller.setGraph(&graph);
        controller.setUndoStack(&undoStack);
        controller.setTypeRegistry(&typeRegistry);

        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("TypeA"),
                                         0, 0, QStringLiteral("#fff"), QStringLiteral("TypeA"));
        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("TypeB"),
                                         100, 100, QStringLiteral("#aaa"), QStringLiteral("TypeB"));
        graph.addComponent(comp1);
        graph.addComponent(comp2);

        QString connId = controller.connectComponentsFromDrag(
            QStringLiteral("comp1"),
            QStringLiteral("comp2"),
            -1, -1,
            QString(),
            QStringLiteral("my_fallback"));

        QVERIFY(!connId.isEmpty());
        ConnectionModel *conn = graph.connectionById(connId);
        QVERIFY(conn);
        // Mock provider normalizes to "normalized", so the normalized value should take precedence
        QCOMPARE(conn->label(), QStringLiteral("normalized"));
    }

    void test_signalBehavior_unchanged()
    {
        ExtensionContractRegistry reg(ExtensionApiVersion{1, 0, 0});
        MockConnectionPolicyProviderPhase4 provider;
        reg.registerConnectionPolicyProvider(&provider);

        TypeRegistry typeRegistry;
        typeRegistry.rebuildFromRegistry(reg);

        GraphModel graph;
        UndoStack undoStack;
        GraphEditorController controller;

        controller.setGraph(&graph);
        controller.setUndoStack(&undoStack);
        controller.setTypeRegistry(&typeRegistry);

        auto *comp1 = new ComponentModel(QStringLiteral("comp1"), QStringLiteral("TypeA"),
                                         0, 0, QStringLiteral("#fff"), QStringLiteral("TypeA"));
        auto *comp2 = new ComponentModel(QStringLiteral("comp2"), QStringLiteral("TypeB"),
                                         100, 100, QStringLiteral("#aaa"), QStringLiteral("TypeB"));
        graph.addComponent(comp1);
        graph.addComponent(comp2);

        QSignalSpy createdSpy(&controller, &GraphEditorController::connectionCreated);
        QSignalSpy rejectedSpy(&controller, &GraphEditorController::connectionRejected);

        controller.connectComponents(QStringLiteral("comp1"), QStringLiteral("comp2"));

        QCOMPARE(createdSpy.count(), 1);
        QCOMPARE(rejectedSpy.count(), 0);

        const QList<QVariant> args = createdSpy.takeFirst();
        QCOMPARE(args.at(1).toString(), QStringLiteral("comp1")); // sourceId
        QCOMPARE(args.at(2).toString(), QStringLiteral("comp2")); // targetId
    }
};

QTEST_MAIN(TestPhase4PolicyMigration)
#include "tst_Phase4PolicyMigration.moc"
