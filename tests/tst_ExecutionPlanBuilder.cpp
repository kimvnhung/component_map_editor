#include <QtTest>

#include "extensions/contracts/builders/ExecutionPlanBuilder.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"

namespace
{
ComponentModel *addComponent(GraphModel &graph, const QString &id)
{
    auto *component = new ComponentModel(&graph);
    component->setId(id);
    component->setType(QStringLiteral("process"));
    graph.addComponent(component);
    return component;
}

void addConnection(GraphModel &graph, const QString &id, const QString &source, const QString &target)
{
    auto *connection = new ConnectionModel(&graph);
    connection->setId(id);
    connection->setSourceId(source);
    connection->setTargetId(target);
    graph.addConnection(connection);
}
}

class tst_ExecutionPlanBuilder : public QObject
{
    Q_OBJECT

private slots:
    void buildsPlanFromGraphStartAndReachability();
    void requestStartOverridesGraphStart();
    void rejectsUnknownStart();
    void rejectsNonRootStartWhenRequired();
    void rejectsDanglingConnections();
    void rejectsDuplicateComponentIds();
};

void tst_ExecutionPlanBuilder::buildsPlanFromGraphStartAndReachability()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("A"));
    addComponent(graph, QStringLiteral("B"));
    addComponent(graph, QStringLiteral("C"));
    addComponent(graph, QStringLiteral("X"));
    addComponent(graph, QStringLiteral("Y"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B"));
    addConnection(graph, QStringLiteral("e2"), QStringLiteral("B"), QStringLiteral("C"));
    addConnection(graph, QStringLiteral("e3"), QStringLiteral("X"), QStringLiteral("Y"));
    graph.setStartComponentId(QStringLiteral("A"));

    ExecutionPlan plan;
    QString error;
    QVERIFY2(ExecutionPlanBuilder::build(&graph, {}, &plan, &error), qPrintable(error));
    QCOMPARE(plan.resolvedStartComponentId, QStringLiteral("A"));
    const QSet<QString> expectedReachable{ QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C") };
    const QSet<QString> expectedIgnored{ QStringLiteral("X"), QStringLiteral("Y") };
    const QSet<QString> expectedConnections{ QStringLiteral("e1"), QStringLiteral("e2") };
    QCOMPARE(plan.reachableComponentIds, expectedReachable);
    QCOMPARE(plan.ignoredComponentIds, expectedIgnored);
    QCOMPARE(plan.activeConnectionIds, expectedConnections);
    QCOMPARE(plan.pendingInDegree.value(QStringLiteral("A")), 0);
    QCOMPARE(plan.pendingInDegree.value(QStringLiteral("B")), 1);
    QCOMPARE(plan.pendingInDegree.value(QStringLiteral("C")), 1);
}

void tst_ExecutionPlanBuilder::requestStartOverridesGraphStart()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("A"));
    addComponent(graph, QStringLiteral("B"));
    addComponent(graph, QStringLiteral("C"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("B"), QStringLiteral("C"));
    graph.setStartComponentId(QStringLiteral("A"));

    ExecutionRequest request;
    request.requestedStartComponentId = QStringLiteral("B");
    ExecutionPlan plan;
    QString error;

    QVERIFY2(ExecutionPlanBuilder::build(&graph, request, &plan, &error), qPrintable(error));
    QCOMPARE(plan.resolvedStartComponentId, QStringLiteral("B"));
    const QSet<QString> expectedReachable{ QStringLiteral("B"), QStringLiteral("C") };
    QCOMPARE(plan.reachableComponentIds, expectedReachable);
    QVERIFY(plan.ignoredComponentIds.contains(QStringLiteral("A")));
}

void tst_ExecutionPlanBuilder::rejectsUnknownStart()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("A"));
    graph.setStartComponentId(QStringLiteral("missing"));

    ExecutionPlan plan;
    QString error;
    QVERIFY(!ExecutionPlanBuilder::build(&graph, {}, &plan, &error));
    QVERIFY(error.contains(QStringLiteral("missing")));
}

void tst_ExecutionPlanBuilder::rejectsNonRootStartWhenRequired()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("A"));
    addComponent(graph, QStringLiteral("B"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("B"));
    graph.setStartComponentId(QStringLiteral("B"));

    ExecutionPlan plan;
    QString error;
    QVERIFY(!ExecutionPlanBuilder::build(&graph, {}, &plan, &error));
    QVERIFY(error.contains(QStringLiteral("incoming")));
}

void tst_ExecutionPlanBuilder::rejectsDanglingConnections()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("A"));
    graph.setStartComponentId(QStringLiteral("A"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("A"), QStringLiteral("missing"));

    ExecutionPlan plan;
    QString error;
    QVERIFY(!ExecutionPlanBuilder::build(&graph, {}, &plan, &error));
    QVERIFY(error.contains(QStringLiteral("e1")));
}

void tst_ExecutionPlanBuilder::rejectsDuplicateComponentIds()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("A"));
    auto *duplicate = new ComponentModel(&graph);
    duplicate->setId(QStringLiteral("A"));
    duplicate->setType(QStringLiteral("process"));
    graph.beginBatchUpdate();
    graph.addComponent(duplicate);
    graph.endBatchUpdate();
    graph.setStartComponentId(QStringLiteral("A"));

    ExecutionPlan plan;
    QString error;
    QVERIFY(!ExecutionPlanBuilder::build(&graph, {}, &plan, &error));
    QVERIFY(error.contains(QStringLiteral("duplicate")));
}

QTEST_MAIN(tst_ExecutionPlanBuilder)
#include "tst_ExecutionPlanBuilder.moc"
