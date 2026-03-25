#include <QtTest>

#include "commands/UndoStack.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/sample_pack/SampleComponentTypeProvider.h"
#include "extensions/sample_pack/SampleConnectionPolicyProvider.h"
#include "models/GraphModel.h"
#include "services/GraphEditorController.h"

struct TestContext {
    ExtensionContractRegistry reg{ExtensionApiVersion{1, 0, 0}};
    SampleComponentTypeProvider componentProvider;
    SampleConnectionPolicyProvider connectionProvider;
    TypeRegistry typeRegistry;
    GraphModel graph;
    UndoStack undoStack;
    GraphEditorController controller;

    TestContext()
    {
        reg.registerComponentTypeProvider(&componentProvider);
        reg.registerConnectionPolicyProvider(&connectionProvider);
        typeRegistry.rebuildFromRegistry(reg);

        controller.setGraph(&graph);
        controller.setUndoStack(&undoStack);
        controller.setTypeRegistry(&typeRegistry);
    }
};

class tst_GraphEditorController : public QObject
{
    Q_OBJECT

private slots:
    void propertiesAreNullByDefault()
    {
        GraphEditorController controller;
        QVERIFY(controller.graph() == nullptr);
        QVERIFY(controller.undoStack() == nullptr);
        QVERIFY(controller.typeRegistry() == nullptr);
    }

    void propertiesCanBeSet()
    {
        GraphModel graph;
        UndoStack stack;
        TypeRegistry typeRegistry;
        GraphEditorController controller;
        controller.setGraph(&graph);
        controller.setUndoStack(&stack);
        controller.setTypeRegistry(&typeRegistry);
        QCOMPARE(controller.graph(), &graph);
        QCOMPARE(controller.undoStack(), &stack);
        QCOMPARE(controller.typeRegistry(), &typeRegistry);
    }

    void createComponentFailsWithoutGraph()
    {
        GraphEditorController controller;
        UndoStack stack;
        controller.setUndoStack(&stack);
        const QString id = controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        QVERIFY(id.isEmpty());
    }

    void createComponentFailsWithoutUndoStack()
    {
        GraphModel graph;
        GraphEditorController controller;
        controller.setGraph(&graph);
        const QString id = controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        QVERIFY(id.isEmpty());
    }

    void createComponentReturnsNonEmptyId()
    {
        TestContext ctx;
        const QString id = ctx.controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 50, 100);
        QVERIFY(!id.isEmpty());
    }

    void createdComponentIsReachableByReturnedId()
    {
        TestContext ctx;
        const QString id = ctx.controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 50, 100);
        QVERIFY(ctx.graph.componentById(id) != nullptr);
    }

    void createComponentAppliesTypeDefaults()
    {
        TestContext ctx;
        const QString id = ctx.controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 50, 100);
        ComponentModel *component = ctx.graph.componentById(id);
        QVERIFY(component != nullptr);
        QCOMPARE(component->width(), 164.0);
        QCOMPARE(component->height(), 100.0);
        QCOMPARE(component->color(), QStringLiteral("#4fc3f7"));
        QCOMPARE(component->type(), QLatin1String(SampleComponentTypeProvider::TypeProcess));
    }

    void createStartComponentAppliesStartDefaults()
    {
        TestContext ctx;
        const QString id = ctx.controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeStart), 0, 0);
        ComponentModel *component = ctx.graph.componentById(id);
        QVERIFY(component != nullptr);
        QCOMPARE(component->width(), 92.0);
        QCOMPARE(component->height(), 92.0);
        QCOMPARE(component->color(), QStringLiteral("#66bb6a"));
    }

    void createComponentWithUnknownTypeUsesFallback()
    {
        TestContext ctx;
        const QString id = ctx.controller.createComponent(QStringLiteral("unknown.type"), 10, 20);
        QVERIFY(!id.isEmpty());
        ComponentModel *component = ctx.graph.componentById(id);
        QVERIFY(component != nullptr);
        QCOMPARE(component->width(), 120.0);
        QCOMPARE(component->height(), 80.0);
    }

    void createComponentWithNoRegistryUsesFallback()
    {
        GraphModel graph;
        UndoStack stack;
        GraphEditorController controller;
        controller.setGraph(&graph);
        controller.setUndoStack(&stack);
        const QString id = controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        QVERIFY(!id.isEmpty());
        ComponentModel *component = graph.componentById(id);
        QVERIFY(component != nullptr);
        QCOMPARE(component->width(), 120.0);
        QCOMPARE(component->height(), 80.0);
    }

    void createComponentPushesUndoableCommand()
    {
        TestContext ctx;
        QVERIFY(!ctx.undoStack.canUndo());
        ctx.controller.createComponent(QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        QVERIFY(ctx.undoStack.canUndo());
    }

    void undoRemovesCreatedComponent()
    {
        TestContext ctx;
        const QString id = ctx.controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        QVERIFY(ctx.graph.componentById(id) != nullptr);

        ctx.undoStack.undo();
        QVERIFY(ctx.graph.componentById(id) == nullptr);
    }

    void redoRecoversCreatedComponent()
    {
        TestContext ctx;
        const QString id = ctx.controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        ctx.undoStack.undo();
        QVERIFY(ctx.graph.componentById(id) == nullptr);

        ctx.undoStack.redo();
        QVERIFY(ctx.graph.componentById(id) != nullptr);
        QCOMPARE(ctx.graph.componentById(id)->type(),
                 QLatin1String(SampleComponentTypeProvider::TypeProcess));
    }

    void createComponentWithIdProducesStableId()
    {
        TestContext ctx;
        const QString wantId = QStringLiteral("my-stable-component-id");
        const QString gotId = ctx.controller.createComponentWithId(
            wantId, QLatin1String(SampleComponentTypeProvider::TypeProcess), 10, 20);
        QCOMPARE(gotId, wantId);
        QVERIFY(ctx.graph.componentById(wantId) != nullptr);
    }

    void componentCreatedSignalEmittedOnSuccess()
    {
        TestContext ctx;
        QSignalSpy spy(&ctx.controller, &GraphEditorController::componentCreated);
        const QString id = ctx.controller.createComponent(
            QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), id);
        QCOMPARE(spy.at(0).at(1).toString(),
                 QLatin1String(SampleComponentTypeProvider::TypeProcess));
    }

    void connectAllowedComponentsReturnsConnectionId()
    {
        TestContext ctx;
        const QString processId1 = ctx.controller.createComponentWithId(
            QStringLiteral("n1"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        const QString processId2 = ctx.controller.createComponentWithId(
            QStringLiteral("n2"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        const QString connId = ctx.controller.connectComponents(processId1, processId2);
        QVERIFY(!connId.isEmpty());
        QVERIFY(ctx.graph.connectionById(connId) != nullptr);
    }

    void connectStartToProcessSucceeds()
    {
        TestContext ctx;
        const QString startId = ctx.controller.createComponentWithId(
            QStringLiteral("s"), QLatin1String(SampleComponentTypeProvider::TypeStart), 0, 0);
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 100, 0);

        QVERIFY(!ctx.controller.connectComponents(startId, processId).isEmpty());
    }

    void connectProcessToStopSucceeds()
    {
        TestContext ctx;
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        const QString stopId = ctx.controller.createComponentWithId(
            QStringLiteral("d"), QLatin1String(SampleComponentTypeProvider::TypeStop), 200, 0);

        QVERIFY(!ctx.controller.connectComponents(processId, stopId).isEmpty());
    }

    void connectStopToProcessFails()
    {
        TestContext ctx;
        const QString stopId = ctx.controller.createComponentWithId(
            QStringLiteral("e"), QLatin1String(SampleComponentTypeProvider::TypeStop), 0, 0);
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        const QString connId = ctx.controller.connectComponents(stopId, processId);
        QVERIFY(connId.isEmpty());
        QCOMPARE(ctx.graph.connectionCount(), 0);
    }

    void connectToStartFails()
    {
        TestContext ctx;
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        const QString startId = ctx.controller.createComponentWithId(
            QStringLiteral("s"), QLatin1String(SampleComponentTypeProvider::TypeStart), 200, 0);

        QVERIFY(ctx.controller.connectComponents(processId, startId).isEmpty());
    }

    void rejectedConnectionEmitsSignal()
    {
        TestContext ctx;
        const QString stopId = ctx.controller.createComponentWithId(
            QStringLiteral("e"), QLatin1String(SampleComponentTypeProvider::TypeStop), 0, 0);
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        QSignalSpy spy(&ctx.controller, &GraphEditorController::connectionRejected);
        ctx.controller.connectComponents(stopId, processId);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), stopId);
        QCOMPARE(spy.at(0).at(1).toString(), processId);
        QVERIFY(!spy.at(0).at(2).toString().isEmpty());
    }

    void lastConnectionRejectionReasonPopulated()
    {
        TestContext ctx;
        const QString stopId = ctx.controller.createComponentWithId(
            QStringLiteral("e"), QLatin1String(SampleComponentTypeProvider::TypeStop), 0, 0);
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        ctx.controller.connectComponents(stopId, processId);
        QVERIFY(!ctx.controller.lastConnectionRejectionReason().isEmpty());
    }

    void successfulConnectClearsRejectionReason()
    {
        TestContext ctx;
        const QString stopId = ctx.controller.createComponentWithId(
            QStringLiteral("e"), QLatin1String(SampleComponentTypeProvider::TypeStop), 0, 0);
        const QString startId = ctx.controller.createComponentWithId(
            QStringLiteral("s"), QLatin1String(SampleComponentTypeProvider::TypeStart), 50, 0);
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        ctx.controller.connectComponents(stopId, processId);
        QVERIFY(!ctx.controller.lastConnectionRejectionReason().isEmpty());

        ctx.controller.connectComponents(startId, processId);
        QVERIFY(ctx.controller.lastConnectionRejectionReason().isEmpty());
    }

    void connectMissingSourceComponentFails()
    {
        TestContext ctx;
        const QString processId = ctx.controller.createComponentWithId(
            QStringLiteral("t"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        QVERIFY(ctx.controller.connectComponents(QStringLiteral("does-not-exist"), processId).isEmpty());
    }

    void connectComponentsPushesUndoableCommand()
    {
        TestContext ctx;
        ctx.controller.createComponentWithId(
            QStringLiteral("n1"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        ctx.controller.createComponentWithId(
            QStringLiteral("n2"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        const int stackBefore = ctx.undoStack.count();
        ctx.controller.connectComponents(QStringLiteral("n1"), QStringLiteral("n2"));
        QCOMPARE(ctx.undoStack.count(), stackBefore + 1);
    }

    void undoRemovesCreatedConnection()
    {
        TestContext ctx;
        ctx.controller.createComponentWithId(
            QStringLiteral("n1"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        ctx.controller.createComponentWithId(
            QStringLiteral("n2"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        const QString connId = ctx.controller.connectComponents(
            QStringLiteral("n1"), QStringLiteral("n2"));
        QVERIFY(ctx.graph.connectionById(connId) != nullptr);

        ctx.undoStack.undo();
        QVERIFY(ctx.graph.connectionById(connId) == nullptr);
    }

    void redoRecoversConnection()
    {
        TestContext ctx;
        ctx.controller.createComponentWithId(
            QStringLiteral("n1"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        ctx.controller.createComponentWithId(
            QStringLiteral("n2"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        const QString connId = ctx.controller.connectComponents(
            QStringLiteral("n1"), QStringLiteral("n2"));
        ctx.undoStack.undo();
        ctx.undoStack.redo();
        QVERIFY(ctx.graph.connectionById(connId) != nullptr);
        QCOMPARE(ctx.graph.connectionById(connId)->sourceId(), QStringLiteral("n1"));
        QCOMPARE(ctx.graph.connectionById(connId)->targetId(), QStringLiteral("n2"));
    }

    void connectionCreatedSignalEmittedOnSuccess()
    {
        TestContext ctx;
        ctx.controller.createComponentWithId(
            QStringLiteral("n1"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 0, 0);
        ctx.controller.createComponentWithId(
            QStringLiteral("n2"), QLatin1String(SampleComponentTypeProvider::TypeProcess), 200, 0);

        QSignalSpy spy(&ctx.controller, &GraphEditorController::connectionCreated);
        const QString connId = ctx.controller.connectComponents(
            QStringLiteral("n1"), QStringLiteral("n2"));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toString(), connId);
        QCOMPARE(spy.at(0).at(1).toString(), QStringLiteral("n1"));
        QCOMPARE(spy.at(0).at(2).toString(), QStringLiteral("n2"));
    }

    void connectWithNoTypeRegistryAlwaysSucceeds()
    {
        GraphModel graph;
        UndoStack stack;
        GraphEditorController controller;
        controller.setGraph(&graph);
        controller.setUndoStack(&stack);

        auto *stopComponent = new ComponentModel(QStringLiteral("e"), QStringLiteral("stop"),
                             0, 0, QStringLiteral("#ef5350"),
                             QStringLiteral("stop"));
        auto *processComponent = new ComponentModel(QStringLiteral("t"), QStringLiteral("process"),
                                 200, 0, QStringLiteral("#4fc3f7"),
                                 QStringLiteral("process"));
        graph.addComponent(stopComponent);
        graph.addComponent(processComponent);
        stack.clear();

        QVERIFY(!controller.connectComponents(QStringLiteral("t"), QStringLiteral("e")).isEmpty());
    }
};

QTEST_MAIN(tst_GraphEditorController)
#include "tst_GraphEditorController.moc"
