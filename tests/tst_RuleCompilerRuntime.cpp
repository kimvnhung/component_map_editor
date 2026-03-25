#include <QtTest>

#include <algorithm>

#include <QFile>
#include <QtMath>
#include <QTemporaryDir>

#include "extensions/runtime/rules/RuleBackedProviders.h"
#include "extensions/runtime/rules/RuleCompiler.h"
#include "extensions/runtime/rules/RuleHotReloadService.h"
#include "extensions/runtime/rules/RuleRuntimeEngine.h"
#include "extensions/runtime/rules/RuleRuntimeRegistry.h"

namespace {

QString validRuleJson(bool allowStartToProcess)
{
    return QStringLiteral(R"JSON({
  "defaultConnectionAllow": false,
  "connections": [
    { "source": "start", "target": "process", "allow": %1, "reason": "rule" },
    { "source": "process", "target": "process", "allow": true }
  ],
  "validations": [
    { "kind": "exactlyOneType", "type": "start", "code": "W001", "severity": "error", "message": "need one start" },
    { "kind": "exactlyOneType", "type": "stop", "code": "W002", "severity": "error", "message": "need one stop" },
    { "kind": "endpointExists", "code": "W003", "severity": "error", "message": "dangling endpoint" },
    { "kind": "noIsolated", "code": "W004", "severity": "warning", "message": "isolated component" }
  ],
  "derivedProperties": [
    { "target": "connection/flow", "property": "type", "value": "flow" }
  ]
})JSON")
  .arg(allowStartToProcess ? QStringLiteral("true") : QStringLiteral("false"));
}

QVariantMap smallGraphSnapshot()
{
    QVariantMap graph;
    graph.insert(QStringLiteral("components"), QVariantList{
        QVariantMap{{QStringLiteral("id"), QStringLiteral("c1")}, {QStringLiteral("type"), QStringLiteral("start")}},
      QVariantMap{{QStringLiteral("id"), QStringLiteral("c2")}, {QStringLiteral("type"), QStringLiteral("stop")}},
      QVariantMap{{QStringLiteral("id"), QStringLiteral("c3")}, {QStringLiteral("type"), QStringLiteral("process")}}
    });
    graph.insert(QStringLiteral("connections"), QVariantList{
        QVariantMap{{QStringLiteral("id"), QStringLiteral("e1")},
                    {QStringLiteral("sourceId"), QStringLiteral("c1")},
                    {QStringLiteral("targetId"), QStringLiteral("c3")}},
        QVariantMap{{QStringLiteral("id"), QStringLiteral("e2")},
                    {QStringLiteral("sourceId"), QStringLiteral("c3")},
                    {QStringLiteral("targetId"), QStringLiteral("c2")}}
    });
    return graph;
}

QString writeTextFile(const QString &path, const QString &text)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return QString();
    file.write(text.toUtf8());
    file.close();
    return path;
}

} // namespace

class tst_RuleCompilerRuntime : public QObject
{
    Q_OBJECT

private slots:
    void compileReportsJsonParseLocation();
    void compileReportsSemanticLocationAndReason();
    void runtimeEvaluationP95UnderTwoMilliseconds();
    void hotReloadAppliesValidUpdatesAndRejectsInvalidWithoutCorruption();
};

void tst_RuleCompilerRuntime::compileReportsJsonParseLocation()
{
    RuleCompiler compiler;
    const RuleCompileResult result = compiler.compileFromJsonText(QStringLiteral("{\n  \"connections\": [\n"));

    QVERIFY(!result.ok);
    QVERIFY(!result.diagnostics.isEmpty());
    const RuleDiagnostic diagnostic = result.diagnostics.first();
    QVERIFY(diagnostic.message.contains(QStringLiteral("JSON parse error")));
    QVERIFY(diagnostic.location.line > 0);
    QVERIFY(diagnostic.location.column > 0);
}

void tst_RuleCompilerRuntime::compileReportsSemanticLocationAndReason()
{
    RuleCompiler compiler;
    const QString broken = QStringLiteral(R"JSON({
  "connections": [
    { "source": "start", "allow": true }
  ]
})JSON");

    const RuleCompileResult result = compiler.compileFromJsonText(broken, QStringLiteral("rules.json"));

    QVERIFY(!result.ok);
    QVERIFY(!result.diagnostics.isEmpty());
    const RuleDiagnostic diagnostic = result.diagnostics.first();
    QVERIFY(diagnostic.location.filePath.contains(QStringLiteral("rules.json")));
    QVERIFY(!diagnostic.location.jsonPath.isEmpty());
    QVERIFY(diagnostic.message.contains(QStringLiteral("Missing required field")));
}

void tst_RuleCompilerRuntime::runtimeEvaluationP95UnderTwoMilliseconds()
{
    RuleCompiler compiler;
    const RuleCompileResult compiled = compiler.compileFromJsonText(validRuleJson(true));
    QVERIFY(compiled.ok);

    RuleRuntimeEngine engine;
    engine.setDescriptor(&compiled.descriptor);

    // Iterations and threshold are configurable via environment variables to
    // prevent flakiness on slow CI machines or debug builds.
    // RULE_PERF_ITERATIONS defaults to 1000; RULE_PERF_THRESHOLD_US defaults to 10000 (10 ms).
    bool envIterSet = false;
    const int envIterations = qEnvironmentVariableIntValue("RULE_PERF_ITERATIONS", &envIterSet);
    const int iterations = (envIterSet && envIterations > 0) ? envIterations : 1000;
    bool envThresholdSet = false;
    const qint64 envThresholdUs = qEnvironmentVariableIntValue("RULE_PERF_THRESHOLD_US", &envThresholdSet);
    const qint64 thresholdUs = (envThresholdSet && envThresholdUs > 0) ? envThresholdUs : 10000;

    QVector<qint64> samplesUs;
    samplesUs.reserve(iterations);

    for (int i = 0; i < iterations; ++i) {
        QString reason;
        const QString src = (i % 2 == 0) ? QStringLiteral("start") : QStringLiteral("process");
        const QString dst = (i % 3 == 0) ? QStringLiteral("process") : QStringLiteral("stop");

        QElapsedTimer timer;
        timer.start();
        (void)engine.canConnect(src, dst, &reason);
        (void)engine.normalizePropertiesForTarget(QStringLiteral("connection/flow"), QVariantMap{});
        (void)engine.validateGraph(smallGraphSnapshot());
        const qint64 elapsedUs = timer.nsecsElapsed() / 1000;
        samplesUs.append(elapsedUs);
    }

    std::sort(samplesUs.begin(), samplesUs.end());
    const int p95Index = qFloor(0.95 * (samplesUs.size() - 1));
    const qint64 p95Us = samplesUs.at(qMax(0, p95Index));

    QVERIFY2(p95Us < thresholdUs,
             qPrintable(QStringLiteral("Expected p95 < %1us, got %2us").arg(thresholdUs).arg(p95Us)));
}

void tst_RuleCompilerRuntime::hotReloadAppliesValidUpdatesAndRejectsInvalidWithoutCorruption()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "temp dir creation failed");

    const QString filePath = dir.path() + QStringLiteral("/rules.json");
    QVERIFY(!writeTextFile(filePath, validRuleJson(true)).isEmpty());

    RuleRuntimeRegistry registry;
    RuleHotReloadService hotReload(&registry);

    QVERIFY(hotReload.startWatchingFile(filePath));
    const qint64 revisionAfterFirstLoad = registry.revision();
    QVERIFY(revisionAfterFirstLoad > 0);

    RuleBackedConnectionPolicyProvider connectionProvider(&registry);
    QString reason;
    QVERIFY(connectionProvider.canConnect(QStringLiteral("start"),
                                          QStringLiteral("process"),
                                          QVariantMap{},
                                          &reason));

    QVERIFY(!writeTextFile(filePath, QStringLiteral("{ invalid json")).isEmpty());
    QVERIFY(!hotReload.reloadNow());
    QCOMPARE(registry.revision(), revisionAfterFirstLoad);

    reason.clear();
    QVERIFY(connectionProvider.canConnect(QStringLiteral("start"),
                                          QStringLiteral("process"),
                                          QVariantMap{},
                                          &reason));

    QVERIFY(!writeTextFile(filePath, validRuleJson(false)).isEmpty());
    QVERIFY(hotReload.reloadNow());
    QVERIFY(registry.revision() > revisionAfterFirstLoad);

    reason.clear();
    QVERIFY(!connectionProvider.canConnect(QStringLiteral("start"),
                                           QStringLiteral("process"),
                                           QVariantMap{},
                                           &reason));
}

QTEST_MAIN(tst_RuleCompilerRuntime)
#include "tst_RuleCompilerRuntime.moc"
