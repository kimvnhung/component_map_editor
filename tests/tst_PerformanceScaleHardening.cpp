#include <QtTest>

#include <algorithm>

#include <QElapsedTimer>
#include <QFile>

#if defined(Q_OS_LINUX)
#  include <unistd.h>
#elif defined(Q_OS_WIN)
#  include <windows.h>
#  include <psapi.h>
#elif defined(Q_OS_MACOS)
#  include <mach/mach.h>
#endif

#include "commands/UndoStack.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/CapabilityRegistry.h"
#include "services/CommandGateway.h"
#include "services/GraphEditorController.h"
#include "services/GraphExecutionSandbox.h"
#include "services/InvariantChecker.h"
#include "services/PerformanceBudgets.h"
#include "services/TraversalEngine.h"
#include "services/ValidationService.h"

namespace {

ComponentModel *makeComponent(GraphModel &graph, const QString &id)
{
    auto *component = new ComponentModel(&graph);
    component->setId(id);
    component->setType(QStringLiteral("process"));
    component->setTitle(id);
    return component;
}

ConnectionModel *makeConnection(GraphModel &graph,
                                const QString &id,
                                const QString &sourceId,
                                const QString &targetId)
{
    auto *connection = new ConnectionModel(&graph);
    connection->setId(id);
    connection->setSourceId(sourceId);
    connection->setTargetId(targetId);
    return connection;
}

void buildScaleGraph(GraphModel &graph, int nodeCount)
{
    graph.beginBatchUpdate();
    for (int i = 0; i < nodeCount; ++i)
        graph.addComponent(makeComponent(graph, QStringLiteral("n%1").arg(i)));

    int connIdx = 0;
    for (int i = 0; i < nodeCount - 1; ++i) {
        graph.addConnection(makeConnection(graph,
                                           QStringLiteral("conn%1").arg(connIdx++),
                                           QStringLiteral("n%1").arg(i),
                                           QStringLiteral("n%1").arg(i + 1)));
        if (i + 5 < nodeCount) {
            graph.addConnection(makeConnection(graph,
                                               QStringLiteral("conn%1").arg(connIdx++),
                                               QStringLiteral("n%1").arg(i),
                                               QStringLiteral("n%1").arg(i + 5)));
        }
    }
    graph.endBatchUpdate();
}

qint64 currentRssBytes()
{
#ifdef Q_OS_LINUX
    QFile file(QStringLiteral("/proc/self/statm"));
    if (!file.open(QIODevice::ReadOnly))
        return -1;

    const QByteArray text = file.readAll().trimmed();
    const QList<QByteArray> fields = text.split(' ');
    if (fields.size() < 2)
        return -1;

    bool ok = false;
    const qint64 residentPages = fields.at(1).toLongLong(&ok);
    if (!ok)
        return -1;

    const long pageSize = ::sysconf(_SC_PAGESIZE);
    if (pageSize <= 0)
        return -1;

    return residentPages * pageSize;
#else
    return -1;
#endif
}

double p95Ms(const QVector<double> &samples)
{
    if (samples.isEmpty())
        return 0.0;

    QVector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const int idx = qBound(0, static_cast<int>(0.95 * (sorted.size() - 1)), sorted.size() - 1);
    return sorted.at(idx);
}

} // namespace

class tst_PerformanceScaleHardening : public QObject
{
    Q_OBJECT

private slots:
    void hardBudgetsAreDefined();
    void commandLatencyP95UnderBudgetForCommonActions();
    void longSessionMemoryGrowthWithinApprovedThreshold();
    // PR5: strict-mode overhead must still be within the latency budget.
    void strictModeOverheadWithinBudget();
};

void tst_PerformanceScaleHardening::hardBudgetsAreDefined()
{
    PerformanceBudgets budgets;
    QCOMPARE(budgets.frameP95BudgetMs(), 16.0);
    QCOMPARE(budgets.commandLatencyP95BudgetMs(), 50.0);
    QCOMPARE(budgets.memoryGrowthBudgetBytes(), 64ll * 1024ll * 1024ll);
}

void tst_PerformanceScaleHardening::commandLatencyP95UnderBudgetForCommonActions()
{
    GraphModel graph;
    UndoStack undo;
    CapabilityRegistry caps;
    CommandGateway gateway;

    gateway.setGraph(&graph);
    gateway.setUndoStack(&undo);
    gateway.setCapabilityRegistry(&caps);

    caps.grantCapabilities(QStringLiteral("bench.plugin"),
                           { QStringLiteral("graph.mutate") });

    QVERIFY(gateway.executeRequest(QStringLiteral("bench.plugin"),
                                   QVariantMap{
                                       { QStringLiteral("command"), QStringLiteral("addComponent") },
                                       { QStringLiteral("id"), QStringLiteral("hot") },
                                       { QStringLiteral("typeId"), QStringLiteral("process") },
                                       { QStringLiteral("x"), 0.0 },
                                       { QStringLiteral("y"), 0.0 }
                                   }));

    gateway.resetCommandLatencyStats();
    const int iterations = 1500;
    for (int i = 0; i < iterations; ++i) {
        const qreal x = static_cast<qreal>(i % 400);
        const qreal y = static_cast<qreal>((i * 3) % 400);
        QVERIFY(gateway.executeRequest(QStringLiteral("bench.plugin"),
                                       QVariantMap{
                                           { QStringLiteral("command"), QStringLiteral("moveComponent") },
                                           { QStringLiteral("id"), QStringLiteral("hot") },
                                           { QStringLiteral("x"), x },
                                           { QStringLiteral("y"), y }
                                       }));
    }

    QVERIFY(gateway.commandLatencySampleCount() >= 100);

    PerformanceBudgets budgets;
    const double p95 = gateway.commandLatencyP95Ms();
    QVERIFY2(budgets.passesCommandLatencyBudget(p95),
             qPrintable(QStringLiteral("Command latency p95 exceeded budget: %1 ms")
                            .arg(p95, 0, 'f', 3)));
}

void tst_PerformanceScaleHardening::longSessionMemoryGrowthWithinApprovedThreshold()
{
    GraphModel graph;
    buildScaleGraph(graph, 1500);

    TraversalEngine traversal;
    traversal.setGraph(&graph);
    traversal.refreshCache();

    ValidationService validation;

    GraphExecutionSandbox sandbox;
    sandbox.setGraph(&graph);

    QVector<double> validationSamples;
    QVector<double> traversalSamples;
    QVector<double> sandboxSamples;
    validationSamples.reserve(80);
    traversalSamples.reserve(80);
    sandboxSamples.reserve(80);

    const qint64 rssBefore = currentRssBytes();

    for (int i = 0; i < 80; ++i) {
        QElapsedTimer timer;

        timer.start();
        (void)validation.validate(&graph);
        validationSamples.append(timer.nsecsElapsed() / 1000000.0);

        timer.restart();
        (void)traversal.bfs({ QStringLiteral("n0") });
        traversalSamples.append(timer.nsecsElapsed() / 1000000.0);

        timer.restart();
        QVERIFY(sandbox.start());
        (void)sandbox.run();
        sandboxSamples.append(timer.nsecsElapsed() / 1000000.0);
    }

    const qint64 rssAfter = currentRssBytes();

    PerformanceBudgets budgets;
    if (rssBefore > 0 && rssAfter > 0) {
        const qint64 delta = rssAfter - rssBefore;
        QVERIFY2(budgets.passesMemoryGrowthBudget(delta),
                 qPrintable(QStringLiteral("Memory growth exceeded approved threshold: %1 bytes")
                                .arg(delta)));
    }

    const double validationP95 = p95Ms(validationSamples);
    const double traversalP95 = p95Ms(traversalSamples);
    const double sandboxP95 = p95Ms(sandboxSamples);

    QVERIFY2(validationP95 < 50.0,
             qPrintable(QStringLiteral("Validation p95 too high: %1 ms").arg(validationP95, 0, 'f', 3)));
    QVERIFY2(traversalP95 < 50.0,
             qPrintable(QStringLiteral("Traversal p95 too high: %1 ms").arg(traversalP95, 0, 'f', 3)));
    QVERIFY2(sandboxP95 < 120.0,
             qPrintable(QStringLiteral("Execution preview p95 too high: %1 ms").arg(sandboxP95, 0, 'f', 3)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  PR5: Strict-mode overhead is within the approved latency budget.
//
//  Strategy: run the same number of createPaletteComponent calls with and
//  without strict mode, each with a non-trivial (but always-passing) invariant.
//  Both p95 values must clear commandLatencyP95BudgetMs (50 ms).  We also
//  assert that strict mode is at most 20× slower than non-strict mode per
//  operation to catch accidental O(n²) invariant evaluation.
// ─────────────────────────────────────────────────────────────────────────────
void tst_PerformanceScaleHardening::strictModeOverheadWithinBudget()
{
    constexpr int kIterations = 500;

    // Helper that runs kIterations createPaletteComponent calls and returns
    // per-call latency samples (ms).
    auto runBench = [&](bool strictMode) -> QVector<double> {
        GraphModel            graph;
        UndoStack             undo;
        InvariantChecker      invariants;
        GraphEditorController controller;

        controller.setGraph(&graph);
        controller.setUndoStack(&undo);
        controller.setInvariantChecker(&invariants);
        controller.setStrictMode(strictMode);

        // Always-passing invariant to exercise the check path in strict mode.
        invariants.registerInvariant(
            QStringLiteral("component-count-sane"),
            [](const GraphModel *g, QString *) -> bool {
                return g->componentCount() <= kIterations + 10;
            });

        QVector<double> samples;
        samples.reserve(kIterations);

        for (int i = 0; i < kIterations; ++i) {
            QElapsedTimer t;
            t.start();
            controller.createPaletteComponent(
                QStringLiteral("process"),
                QStringLiteral("Node"),
                {},
                {},
                static_cast<qreal>(i % 600),
                static_cast<qreal>((i * 3) % 600),
                96.0,
                96.0);
            samples.append(static_cast<double>(t.nsecsElapsed()) / 1'000'000.0);
        }
        return samples;
    };

    const QVector<double> nonStrictSamples = runBench(/*strictMode=*/false);
    const QVector<double> strictSamples    = runBench(/*strictMode=*/true);

    const double nonStrictP95 = p95Ms(nonStrictSamples);
    const double strictP95    = p95Ms(strictSamples);

    PerformanceBudgets budgets;

    QVERIFY2(budgets.passesCommandLatencyBudget(nonStrictP95),
             qPrintable(QStringLiteral("Non-strict p95 exceeded budget: %1 ms")
                            .arg(nonStrictP95, 0, 'f', 3)));
    QVERIFY2(budgets.passesCommandLatencyBudget(strictP95),
             qPrintable(QStringLiteral("Strict-mode p95 exceeded budget: %1 ms")
                            .arg(strictP95, 0, 'f', 3)));

    // Overhead ratio guard: strict mode must not be more than 20× slower.
    // This guards against accidentally calling checkAll() in a loop or with
    // an O(n) invariant inside each mutation.
    if (nonStrictP95 > 0.0) {
        const double ratio = strictP95 / nonStrictP95;
        QVERIFY2(ratio < 20.0,
                 qPrintable(QStringLiteral("Strict-mode overhead ratio too high: %1× "
                                           "(strict=%2 ms, non-strict=%3 ms)")
                                .arg(ratio, 0, 'f', 1)
                                .arg(strictP95, 0, 'f', 3)
                                .arg(nonStrictP95, 0, 'f', 3)));
    }
}

QTEST_MAIN(tst_PerformanceScaleHardening)
#include "tst_PerformanceScaleHardening.moc"
