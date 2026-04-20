#include <QtTest>

#include "customizeexecutionsanticsprovider.h"

#include "models/ComponentModel.h"
#include "models/GraphModel.h"
#include "services/ExecutionMigrationFlags.h"
#include "services/GraphExecutionSandbox.h"

namespace {

ComponentModel *makeComponent(GraphModel &graph,
                              const QString &id,
                              const QString &type,
                              const QVariantMap &props = {})
{
    auto *component = new ComponentModel(&graph);
    component->setId(id);
    component->setType(type);
    component->setTitle(id);
    for (auto it = props.constBegin(); it != props.constEnd(); ++it)
        component->setDynamicProperty(it.key(), it.value());
    return component;
}

void runSingleNode(GraphExecutionSandbox &sandbox,
                   const QString &type,
                   const QVariantMap &props,
                   const QVariantMap &input)
{
    auto *graph = new GraphModel(&sandbox);
    graph->addComponent(makeComponent(*graph, QStringLiteral("n1"), type, props));
    sandbox.setGraph(graph);
    QVERIFY(sandbox.start(input));
    sandbox.run();
}

} // namespace

class tst_CustomizeExampleMathWorkflows : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void basicComponents_computeAndWriteContext();
    void mathProviders_supportConfigurableOutputAndErrorKeys();
    void controlIfElse_routesTrueAndFalse();
    void controlLoop_stopsByMaxIterAndCondition();
    void divideByZero_reportsError();
    void executionTrace_containsInputsOutputsAndErrors();
    void duplicateIncomingKeys_disambiguatedByInputRef();
    void duplicateIncomingKeys_plainKeyFailsWithoutInputRef();
    void directFallbackWithoutRefs_usesSnapshotFallbacks();
    void explicitFallbackSentinel_usesSnapshotFallbacks();
    void plainKeyFallback_remainsBackwardCompatible();

private:
    CustomizeExecutionSemanticsProvider m_provider;
};

void tst_CustomizeExampleMathWorkflows::init()
{
    cme::execution::MigrationFlags::resetDefaults();
    cme::execution::MigrationFlags::setTokenTransportEnabled(true);
}

void tst_CustomizeExampleMathWorkflows::cleanup()
{
    cme::execution::MigrationFlags::resetDefaults();
}

void tst_CustomizeExampleMathWorkflows::basicComponents_computeAndWriteContext()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
                  QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("__graph_input__::a")},
                              {QStringLiteral("inputBRef"), QStringLiteral("__graph_input__::b")},
                              {QStringLiteral("outputKey"), QStringLiteral("total")}},
                  QVariantMap{{QStringLiteral("a"), 3}, {QStringLiteral("b"), 2}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("total")).toDouble(), 5.0);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSubtract),
                  QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("__graph_input__::a")},
                              {QStringLiteral("inputBRef"), QStringLiteral("__graph_input__::b")},
                              {QStringLiteral("outputKey"), QStringLiteral("diff")}},
                  QVariantMap{{QStringLiteral("a"), 7}, {QStringLiteral("b"), 2}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("diff")).toDouble(), 5.0);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeMultiply),
                  QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("__graph_input__::a")},
                              {QStringLiteral("inputBRef"), QStringLiteral("__graph_input__::b")},
                              {QStringLiteral("outputKey"), QStringLiteral("product")}},
                  QVariantMap{{QStringLiteral("a"), 3}, {QStringLiteral("b"), 4}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("product")).toDouble(), 12.0);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeDivide),
                  QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("__graph_input__::a")},
                              {QStringLiteral("inputBRef"), QStringLiteral("__graph_input__::b")},
                              {QStringLiteral("outputKey"), QStringLiteral("quotient")}},
                  QVariantMap{{QStringLiteral("a"), 12}, {QStringLiteral("b"), 3}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("quotient")).toDouble(), 4.0);
}

void tst_CustomizeExampleMathWorkflows::mathProviders_supportConfigurableOutputAndErrorKeys()
{
    QVariantMap output;
    QVariantMap trace;
    QString error;

    bool ok = m_provider.executeComponent(
        QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
        QStringLiteral("add-custom-output"),
        QVariantMap{{QStringLiteral("a"), 5.0},
                    {QStringLiteral("b"), 6.0},
                    {QStringLiteral("outputKey"), QStringLiteral("customSum")}},
        cme::execution::IncomingTokens{},
        &output,
        &trace,
        &error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(output.value(QStringLiteral("customSum")).toDouble(), 11.0);

    output.clear();
    trace.clear();
    error.clear();
    ok = m_provider.executeComponent(
        QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSubtract),
        QStringLiteral("subtract-custom-error"),
        QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("missing::a")},
                    {QStringLiteral("inputBRef"), QStringLiteral("missing::b")},
                    {QStringLiteral("errorKey"), QStringLiteral("customError")}},
        cme::execution::IncomingTokens{},
        &output,
        &trace,
        &error);

    QVERIFY(!ok);
    QCOMPARE(output.value(QStringLiteral("customError")).toString(), error);
    QVERIFY(trace.value(QStringLiteral("outputs")).toMap().contains(QStringLiteral("customError")));
}

void tst_CustomizeExampleMathWorkflows::controlIfElse_routesTrueAndFalse()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeIfElse),
                  QVariantMap{{QStringLiteral("conditionKey"), QStringLiteral("cond")},
                              {QStringLiteral("trueRouteKey"), QStringLiteral("goTrue")},
                              {QStringLiteral("falseRouteKey"), QStringLiteral("goFalse")}},
                  QVariantMap{{QStringLiteral("cond"), true}});
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(sandbox.executionState().value(QStringLiteral("goTrue")).toBool(), true);
    QCOMPARE(sandbox.executionState().value(QStringLiteral("goFalse")).toBool(), false);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeIfElse),
                  QVariantMap{{QStringLiteral("conditionKey"), QStringLiteral("cond")},
                              {QStringLiteral("trueRouteKey"), QStringLiteral("goTrue")},
                              {QStringLiteral("falseRouteKey"), QStringLiteral("goFalse")}},
                  QVariantMap{{QStringLiteral("cond"), false}});
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(sandbox.executionState().value(QStringLiteral("goTrue")).toBool(), false);
    QCOMPARE(sandbox.executionState().value(QStringLiteral("goFalse")).toBool(), true);
}

void tst_CustomizeExampleMathWorkflows::controlLoop_stopsByMaxIterAndCondition()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeLoop),
                  QVariantMap{},
                  QVariantMap{{QStringLiteral("iter"), 0},
                              {QStringLiteral("maxIter"), 3},
                              {QStringLiteral("condition"), true}});
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(sandbox.executionState().value(QStringLiteral("iter")).toInt(), 1);
    QCOMPARE(sandbox.executionState().value(QStringLiteral("continueLoop")).toBool(), true);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeLoop),
                  QVariantMap{},
                  QVariantMap{{QStringLiteral("iter"), 2},
                              {QStringLiteral("maxIter"), 3},
                              {QStringLiteral("condition"), true}});
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(sandbox.executionState().value(QStringLiteral("iter")).toInt(), 3);
    QCOMPARE(sandbox.executionState().value(QStringLiteral("continueLoop")).toBool(), false);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeLoop),
                  QVariantMap{},
                  QVariantMap{{QStringLiteral("iter"), 0},
                              {QStringLiteral("maxIter"), 5},
                              {QStringLiteral("condition"), false}});
    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    QCOMPARE(sandbox.executionState().value(QStringLiteral("continueLoop")).toBool(), false);
}

void tst_CustomizeExampleMathWorkflows::divideByZero_reportsError()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeDivide),
                  QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("__graph_input__::a")},
                              {QStringLiteral("inputBRef"), QStringLiteral("__graph_input__::b")},
                              {QStringLiteral("errorKey"), QStringLiteral("error")}},
                  QVariantMap{{QStringLiteral("a"), 8}, {QStringLiteral("b"), 0}});

    QCOMPARE(sandbox.status(), QStringLiteral("error"));
    QVERIFY(sandbox.lastError().contains(QStringLiteral("Division by zero")));
}

void tst_CustomizeExampleMathWorkflows::executionTrace_containsInputsOutputsAndErrors()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
                  QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("__graph_input__::a")},
                              {QStringLiteral("inputBRef"), QStringLiteral("__graph_input__::b")},
                              {QStringLiteral("outputKey"), QStringLiteral("sum")}},
                  QVariantMap{{QStringLiteral("a"), 8.0}, {QStringLiteral("b"), 3.0}});

    const QVariantList timeline = sandbox.timeline();
    bool sawStepWithTrace = false;
    for (const QVariant &entryVar : timeline) {
        const QVariantMap entry = entryVar.toMap();
        if (entry.value(QStringLiteral("event")).toString() != QStringLiteral("stepExecuted"))
            continue;

        const QVariantMap trace = entry.value(QStringLiteral("trace")).toMap();
        if (trace.isEmpty())
            continue;

        sawStepWithTrace = true;
        QVERIFY(trace.contains(QStringLiteral("inputs")));
        QVERIFY(trace.contains(QStringLiteral("outputs")));
    }

    QVERIFY(sawStepWithTrace);
}

void tst_CustomizeExampleMathWorkflows::duplicateIncomingKeys_disambiguatedByInputRef()
{
    cme::execution::IncomingTokens incomingTokens;
    incomingTokens.insert(QStringLiteral("edge.sourceA.crc"),
                          QVariantMap{{QStringLiteral("rd_key1"), 3.0},
                                      {QStringLiteral("b"), 2.0}});
    incomingTokens.insert(QStringLiteral("edge.sourceB.crc"),
                          QVariantMap{{QStringLiteral("rd_key1"), 9.0},
                                      {QStringLiteral("b"), 2.0}});

    QVariantMap output;
    QVariantMap trace;
    QString error;
    const bool ok = m_provider.executeComponent(
        QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
        QStringLiteral("crc"),
        QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("edge.sourceB.crc::rd_key1")},
                    {QStringLiteral("inputBRef"), QStringLiteral("edge.sourceB.crc::b")},
                    {QStringLiteral("outputKey"), QStringLiteral("sum")}},
        incomingTokens,
        &output,
        &trace,
        &error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(output.value(QStringLiteral("sum")).toDouble(), 11.0);
}

void tst_CustomizeExampleMathWorkflows::duplicateIncomingKeys_plainKeyFailsWithoutInputRef()
{
    cme::execution::IncomingTokens incomingTokens;
    incomingTokens.insert(QStringLiteral("edge.sourceA.crc"),
                          QVariantMap{{QStringLiteral("rd_key1"), 3.0},
                                      {QStringLiteral("b"), 2.0}});
    incomingTokens.insert(QStringLiteral("edge.sourceB.crc"),
                          QVariantMap{{QStringLiteral("rd_key1"), 9.0},
                                      {QStringLiteral("b"), 2.0}});

    QVariantMap output;
    QVariantMap trace;
    QString error;
    const bool ok = m_provider.executeComponent(
        QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
        QStringLiteral("crc"),
        QVariantMap{{QStringLiteral("inputAKey"), QStringLiteral("rd_key1")},
                    {QStringLiteral("inputBKey"), QStringLiteral("b")},
                    {QStringLiteral("outputKey"), QStringLiteral("sum")}},
        incomingTokens,
        &output,
        &trace,
        &error);

    QVERIFY(!ok);
    QVERIFY(error.contains(QStringLiteral("Invalid numeric input")));
}

void tst_CustomizeExampleMathWorkflows::directFallbackWithoutRefs_usesSnapshotFallbacks()
{
    QVariantMap output;
    QVariantMap trace;
    QString error;
    const bool ok = m_provider.executeComponent(
        QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSubtract),
        QStringLiteral("fallback-node"),
        QVariantMap{{QStringLiteral("a"), 10.0},
                    {QStringLiteral("b"), 4.0},
                    {QStringLiteral("outputKey"), QStringLiteral("difference")}},
        cme::execution::IncomingTokens{},
        &output,
        &trace,
        &error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(output.value(QStringLiteral("difference")).toDouble(), 6.0);
}

void tst_CustomizeExampleMathWorkflows::explicitFallbackSentinel_usesSnapshotFallbacks()
{
    QVariantMap output;
    QVariantMap trace;
    QString error;
    const bool ok = m_provider.executeComponent(
        QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
        QStringLiteral("fallback-sentinel-node"),
        QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("__use_fallback__")},
                    {QStringLiteral("inputBRef"), QStringLiteral("__graph_input__::b")},
                    {QStringLiteral("a"), 10.0},
                    {QStringLiteral("outputKey"), QStringLiteral("sum")}},
        cme::execution::IncomingTokens{{QStringLiteral("__graph_input__"),
                                        QVariantMap{{QStringLiteral("b"), 4.0}}}},
        &output,
        &trace,
        &error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(output.value(QStringLiteral("sum")).toDouble(), 14.0);
}

void tst_CustomizeExampleMathWorkflows::plainKeyFallback_remainsBackwardCompatible()
{
    cme::execution::IncomingTokens incomingTokens;
    incomingTokens.insert(QStringLiteral("edge.sourceA.crc"),
                          QVariantMap{{QStringLiteral("a"), 4.0},
                                      {QStringLiteral("b"), 6.0}});

    QVariantMap output;
    QVariantMap trace;
    QString error;
    const bool ok = m_provider.executeComponent(
        QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
        QStringLiteral("crc"),
        QVariantMap{{QStringLiteral("inputAKey"), QStringLiteral("a")},
                    {QStringLiteral("inputBKey"), QStringLiteral("b")},
                    {QStringLiteral("outputKey"), QStringLiteral("sum")}},
        incomingTokens,
        &output,
        &trace,
        &error);

    QVERIFY2(ok, qPrintable(error));
    QCOMPARE(output.value(QStringLiteral("sum")).toDouble(), 10.0);
}

QTEST_MAIN(tst_CustomizeExampleMathWorkflows)
#include "tst_CustomizeExampleMathWorkflows.moc"
