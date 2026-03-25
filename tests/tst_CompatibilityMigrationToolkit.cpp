#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QFile>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/ExtensionCompatibilityChecker.h"
#include "extensions/runtime/ExtensionManifestJson.h"
#include "extensions/sample_pack/LegacyV0ExtensionPack.h"
#include "extensions/sample_pack/SampleExtensionPack.h"

namespace {

QString writeManifest(const QString &directory,
                      const QString &fileName,
                      const QJsonObject &manifestObject)
{
    const QString path = directory + QLatin1Char('/') + fileName;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return {};

    file.write(QJsonDocument(manifestObject).toJson(QJsonDocument::Indented));
    file.close();
    return path;
}

QJsonObject legacyManifestObject()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("extensionId"), QStringLiteral("sample.workflow.legacy-v0"));
    obj.insert(QStringLiteral("displayName"), QStringLiteral("Sample Workflow Pack (Legacy v0)"));
    obj.insert(QStringLiteral("extensionVersion"), QStringLiteral("0.9.0"));
    obj.insert(QStringLiteral("minCoreApi"), QStringLiteral("1.0.0"));
    obj.insert(QStringLiteral("maxCoreApi"), QStringLiteral("1.99.99"));

    QJsonArray capabilities;
    capabilities.append(QStringLiteral("executionSemantics"));
    obj.insert(QStringLiteral("capabilities"), capabilities);
    obj.insert(QStringLiteral("dependencies"), QJsonArray{});

    QJsonObject metadata;
    QJsonObject contractVersions;
    contractVersions.insert(QStringLiteral("executionSemantics"), 0);
    metadata.insert(QStringLiteral("contractVersions"), contractVersions);
    obj.insert(QStringLiteral("metadata"), metadata);
    return obj;
}

QJsonObject modernManifestObject()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("extensionId"), QStringLiteral("sample.workflow"));
    obj.insert(QStringLiteral("displayName"), QStringLiteral("Sample Workflow Pack"));
    obj.insert(QStringLiteral("extensionVersion"), QStringLiteral("1.2.3"));
    obj.insert(QStringLiteral("minCoreApi"), QStringLiteral("1.0.0"));
    obj.insert(QStringLiteral("maxCoreApi"), QStringLiteral("1.99.99"));

    QJsonArray capabilities;
    capabilities.append(QStringLiteral("executionSemantics"));
    capabilities.append(QStringLiteral("actions"));
    capabilities.append(QStringLiteral("propertySchema"));
    obj.insert(QStringLiteral("capabilities"), capabilities);
    obj.insert(QStringLiteral("dependencies"), QJsonArray{});

    QJsonObject metadata;
    QJsonObject contractVersions;
    contractVersions.insert(QStringLiteral("executionSemantics"), 1);
    contractVersions.insert(QStringLiteral("actions"), 1);
    contractVersions.insert(QStringLiteral("propertySchema"), 1);
    metadata.insert(QStringLiteral("contractVersions"), contractVersions);
    obj.insert(QStringLiteral("metadata"), metadata);
    return obj;
}

QStringList reportIds(const QVariantList &items)
{
    QStringList ids;
    for (const QVariant &v : items)
        ids.append(v.toMap().value(QStringLiteral("id")).toString());
    ids.sort();
    return ids;
}

} // namespace

class CompatibilityMigrationToolkitTests : public QObject
{
    Q_OBJECT

private slots:
    void legacyExecutionPackRegistersViaAdapterWithoutCoreEdits();
    void compatibilityCheckerCatchesAllIntentionalBreakings();
    void compatibilityCheckerReportsDeprecatedApis();
    void modernPackReportHasNoBreakingChanges();
};

void CompatibilityMigrationToolkitTests::legacyExecutionPackRegistersViaAdapterWithoutCoreEdits()
{
    ExtensionContractRegistry registry({1, 0, 0});
    LegacyV0ExtensionPack legacyPack;

    QString error;
    QVERIFY2(legacyPack.registerAll(registry, &error), qPrintable(error));

    const QList<const IExecutionSemanticsProvider *> providers =
        registry.executionSemanticsProviders();
    QCOMPARE(providers.size(), 1);
    QCOMPARE(providers.first()->providerId(),
             QStringLiteral("sample.workflow.execution.legacy.v0"));

    QVariantMap output;
    QVariantMap trace;
    QString execError;
    QVERIFY(providers.first()->executeComponent(QStringLiteral("start"),
                                                QStringLiteral("t-1"),
                                                QVariantMap{},
                                                QVariantMap{},
                                                &output,
                                                &trace,
                                                &execError));
    QVERIFY(execError.isEmpty());
    QCOMPARE(output.value(QStringLiteral("started")).toBool(), true);
    QCOMPARE(trace.value(QStringLiteral("adapter")).toString(),
             QStringLiteral("executionSemantics.v0"));
}

void CompatibilityMigrationToolkitTests::compatibilityCheckerCatchesAllIntentionalBreakings()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString path = writeManifest(dir.path(),
                                       QStringLiteral("legacy.json"),
                                       legacyManifestObject());
    QVERIFY(!path.isEmpty());

    ExtensionCompatibilityChecker checker({1, 0, 0});
    QVariantMap report;
    QString error;
    QVERIFY2(checker.analyzeManifestFile(path, &report, &error), qPrintable(error));

    const QStringList expected = ExtensionCompatibilityChecker::intentionalBreakingChangeIds();
    QStringList actual = reportIds(report.value(QStringLiteral("breaking")).toList());

    QStringList sortedExpected = expected;
    sortedExpected.sort();
    QCOMPARE(actual, sortedExpected);
}

void CompatibilityMigrationToolkitTests::compatibilityCheckerReportsDeprecatedApis()
{
    ExtensionCompatibilityChecker checker({1, 0, 0});

    ExtensionManifest manifest;
    manifest.extensionId = QStringLiteral("legacy.deprecated");
    manifest.displayName = QStringLiteral("Legacy Deprecation Pack");
    manifest.extensionVersion = QStringLiteral("0.8.0");
    manifest.minCoreApi = {1, 0, 0};
    manifest.maxCoreApi = {1, 99, 99};
    manifest.capabilities = {
        QStringLiteral("actions"),
        QStringLiteral("propertySchema")
    };

    QVariantMap contractVersions;
    contractVersions.insert(QStringLiteral("actions"), 0);
    contractVersions.insert(QStringLiteral("propertySchema"), 0);
    manifest.metadata.insert(QStringLiteral("contractVersions"), contractVersions);

    const QVariantMap report = checker.analyzeManifest(manifest);
    const QVariantList deprecated = report.value(QStringLiteral("deprecated")).toList();
    QVERIFY(deprecated.size() >= 2);

    const QStringList ids = reportIds(deprecated);
    QVERIFY(ids.contains(QStringLiteral("EXT-ACTION-DEP-001")));
    QVERIFY(ids.contains(QStringLiteral("EXT-SCHEMA-DEP-001")));
}

void CompatibilityMigrationToolkitTests::modernPackReportHasNoBreakingChanges()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString path = writeManifest(dir.path(),
                                       QStringLiteral("modern.json"),
                                       modernManifestObject());
    QVERIFY(!path.isEmpty());

    ExtensionCompatibilityChecker checker({1, 0, 0});
    QVariantMap report;
    QString error;
    QVERIFY2(checker.analyzeManifestFile(path, &report, &error), qPrintable(error));

    QCOMPARE(report.value(QStringLiteral("breakingCount")).toInt(), 0);
    QVERIFY(report.value(QStringLiteral("compatible")).toBool());
}

QTEST_MAIN(CompatibilityMigrationToolkitTests)
#include "tst_CompatibilityMigrationToolkit.moc"
