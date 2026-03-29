#include <QtTest>
#include <cmath>

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
    void divideByZero_reportsError();
    void sqrtNewton_loopStatePersistenceAndConvergence();
    void quadratic_branchesAndReusesSqrtLogic();
    void sqrtNegative_reportsError();
    void executionTrace_containsInputsOutputsAndErrors();

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
                  QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("sum")}},
                  QVariantMap{{QStringLiteral("a"), 3}, {QStringLiteral("b"), 2}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("sum")).toDouble(), 5.0);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSubtract),
                  QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("diff")}},
                  QVariantMap{{QStringLiteral("a"), 7}, {QStringLiteral("b"), 2}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("diff")).toDouble(), 5.0);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeMultiply),
                  QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("product")}},
                  QVariantMap{{QStringLiteral("a"), 3}, {QStringLiteral("b"), 4}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("product")).toDouble(), 12.0);

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeDivide),
                  QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("quotient")}},
                  QVariantMap{{QStringLiteral("a"), 12}, {QStringLiteral("b"), 3}});
    QCOMPARE(sandbox.executionState().value(QStringLiteral("quotient")).toDouble(), 4.0);
}

void tst_CustomizeExampleMathWorkflows::divideByZero_reportsError()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeDivide),
                  QVariantMap{{QStringLiteral("errorKey"), QStringLiteral("error")}},
                  QVariantMap{{QStringLiteral("a"), 8}, {QStringLiteral("b"), 0}});

    QCOMPARE(sandbox.status(), QStringLiteral("error"));
    QVERIFY(sandbox.lastError().contains(QStringLiteral("Division by zero")));
}

void tst_CustomizeExampleMathWorkflows::sqrtNewton_loopStatePersistenceAndConvergence()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSqrtNewton),
                  QVariantMap{{QStringLiteral("sKey"), QStringLiteral("S")},
                              {QStringLiteral("epsilonKey"), QStringLiteral("epsilon")},
                              {QStringLiteral("outputKey"), QStringLiteral("sqrtS")},
                              {QStringLiteral("maxIterations"), 64}},
                  QVariantMap{{QStringLiteral("S"), 25.0}, {QStringLiteral("epsilon"), 1e-8}});

    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    const QVariantMap state = sandbox.executionState();
    const double sqrtS = state.value(QStringLiteral("sqrtS")).toDouble();
    QVERIFY(std::abs(sqrtS - 5.0) < 1e-5);
    QVERIFY(state.value(QStringLiteral("sqrt.iterations")).toInt() > 0);
    QVERIFY(state.value(QStringLiteral("sqrt.lastDelta")).toDouble() < 1e-4);
}

void tst_CustomizeExampleMathWorkflows::quadratic_branchesAndReusesSqrtLogic()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeQuadratic),
                  QVariantMap{},
                  QVariantMap{{QStringLiteral("a"), 1.0},
                              {QStringLiteral("b"), -3.0},
                              {QStringLiteral("c"), 2.0},
                              {QStringLiteral("epsilon"), 1e-8}});

    QCOMPARE(sandbox.status(), QStringLiteral("completed"));
    const QVariantMap state = sandbox.executionState();
    QCOMPARE(state.value(QStringLiteral("quadratic.status")).toString(), QStringLiteral("two_roots"));
    const double x1 = state.value(QStringLiteral("x1")).toDouble();
    const double x2 = state.value(QStringLiteral("x2")).toDouble();
    QVERIFY((std::abs(x1 - 2.0) < 1e-4 && std::abs(x2 - 1.0) < 1e-4)
            || (std::abs(x1 - 1.0) < 1e-4 && std::abs(x2 - 2.0) < 1e-4));

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeQuadratic),
                  QVariantMap{},
                  QVariantMap{{QStringLiteral("a"), 1.0},
                              {QStringLiteral("b"), 2.0},
                              {QStringLiteral("c"), 5.0}});

    QCOMPARE(sandbox.status(), QStringLiteral("error"));
    QVERIFY(sandbox.lastError().contains(QStringLiteral("No real roots")));
}

void tst_CustomizeExampleMathWorkflows::sqrtNegative_reportsError()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSqrtNewton),
                  QVariantMap{},
                  QVariantMap{{QStringLiteral("S"), -4.0}, {QStringLiteral("epsilon"), 1e-6}});

    QCOMPARE(sandbox.status(), QStringLiteral("error"));
    QVERIFY(sandbox.lastError().contains(QStringLiteral("Square root of negative number")));
}

void tst_CustomizeExampleMathWorkflows::executionTrace_containsInputsOutputsAndErrors()
{
    GraphExecutionSandbox sandbox;
    sandbox.setExecutionSemanticsProviders({ &m_provider });

    runSingleNode(sandbox,
                  QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd),
                  QVariantMap{{QStringLiteral("outputKey"), QStringLiteral("sum")}},
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

QTEST_MAIN(tst_CustomizeExampleMathWorkflows)
#include "tst_CustomizeExampleMathWorkflows.moc"
