#include <QtTest/QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/sample_pack/SampleExtensionPack.h"

namespace {

QString writeManifest(const QString &directory,
                      const QString &fileName,
                      const QJsonObject &manifestObject)
{
    const QString path = directory + QLatin1Char('/') + fileName;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return QString();
    }

    file.write(QJsonDocument(manifestObject).toJson(QJsonDocument::Indented));
    file.close();
    return path;
}

QJsonObject validSampleManifestObject()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("extensionId"), QStringLiteral("sample.workflow"));
    obj.insert(QStringLiteral("displayName"), QStringLiteral("Sample Workflow Pack From Manifest"));
    obj.insert(QStringLiteral("extensionVersion"), QStringLiteral("1.2.3"));
    obj.insert(QStringLiteral("minCoreApi"), QStringLiteral("1.0.0"));
    obj.insert(QStringLiteral("maxCoreApi"), QStringLiteral("1.99.99"));

    QJsonArray capabilities;
    capabilities.append(QStringLiteral("nodeTypes"));
    capabilities.append(QStringLiteral("connectionPolicy"));
    capabilities.append(QStringLiteral("propertySchema"));
    capabilities.append(QStringLiteral("validation"));
    capabilities.append(QStringLiteral("actions"));
    obj.insert(QStringLiteral("capabilities"), capabilities);

    obj.insert(QStringLiteral("dependencies"), QJsonArray{});

    QJsonObject metadata;
    metadata.insert(QStringLiteral("owner"), QStringLiteral("qa-phase2"));
    metadata.insert(QStringLiteral("description"), QStringLiteral("Manifest-only loading verification"));
    obj.insert(QStringLiteral("metadata"), metadata);

    return obj;
}

bool diagnosticsContain(const ExtensionLoadResult &result, const QString &fragment)
{
    for (const ExtensionLoadDiagnostic &d : result.diagnostics) {
        if (d.message.contains(fragment, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

} // namespace

class ExtensionStartupLoaderTests : public QObject
{
    Q_OBJECT

private slots:
    void discoversAndLoadsSamplePackFromManifestOnly();
    void invalidManifestFailsFastWithActionableDiagnostics();
    void loaderStaysStableWhenOnePackFails();
    void missingFactoryProducesClearError();
    void missingDependencyProducesClearError();
};

void ExtensionStartupLoaderTests::discoversAndLoadsSamplePackFromManifestOnly()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "temp dir creation failed");

    const QString path = writeManifest(dir.path(), QStringLiteral("sample.workflow.json"),
                                       validSampleManifestObject());
    QVERIFY2(!path.isEmpty(), "manifest write failed");

    ExtensionContractRegistry registry({1, 0, 0});
    ExtensionStartupLoader loader;
    loader.registerFactory(QStringLiteral("sample.workflow"), []() {
        return std::make_unique<SampleExtensionPack>();
    });

    const ExtensionLoadResult result = loader.loadFromDirectory(dir.path(), registry);

    QCOMPARE(result.discoveredManifestCount, 1);
    QCOMPARE(result.loadedPackCount, 1);
    QVERIFY(!result.hasErrors());
    QVERIFY(registry.hasManifest(QStringLiteral("sample.workflow")));
    QVERIFY(result.loadedExtensionIds.contains(QStringLiteral("sample.workflow")));

    const ExtensionManifest manifest = registry.manifest(QStringLiteral("sample.workflow"));
    QCOMPARE(manifest.displayName, QStringLiteral("Sample Workflow Pack From Manifest"));
    QCOMPARE(manifest.extensionVersion, QStringLiteral("1.2.3"));
    QCOMPARE(manifest.metadata.value(QStringLiteral("owner")).toString(), QStringLiteral("qa-phase2"));
}

void ExtensionStartupLoaderTests::invalidManifestFailsFastWithActionableDiagnostics()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "temp dir creation failed");

    QJsonObject invalid;
    invalid.insert(QStringLiteral("displayName"), QStringLiteral("Missing Id"));
    invalid.insert(QStringLiteral("extensionVersion"), QStringLiteral("1.0.0"));
    invalid.insert(QStringLiteral("minCoreApi"), QStringLiteral("1.0.0"));
    invalid.insert(QStringLiteral("maxCoreApi"), QStringLiteral("1.0.0"));

    QVERIFY(!writeManifest(dir.path(), QStringLiteral("invalid.json"), invalid).isEmpty());

    ExtensionContractRegistry registry({1, 0, 0});
    ExtensionStartupLoader loader;

    const ExtensionLoadResult result = loader.loadFromDirectory(dir.path(), registry);

    QCOMPARE(result.discoveredManifestCount, 1);
    QCOMPARE(result.loadedPackCount, 0);
    QVERIFY(result.hasErrors());
    QVERIFY(diagnosticsContain(result, QStringLiteral("extensionId")));
    QVERIFY(diagnosticsContain(result, QStringLiteral("Manifest parse failed")));
}

void ExtensionStartupLoaderTests::loaderStaysStableWhenOnePackFails()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "temp dir creation failed");

    QVERIFY(!writeManifest(dir.path(), QStringLiteral("valid.json"),
                           validSampleManifestObject()).isEmpty());

    QJsonObject invalid = validSampleManifestObject();
    invalid.insert(QStringLiteral("extensionId"), QStringLiteral("broken.pack"));
    invalid.insert(QStringLiteral("minCoreApi"), QStringLiteral("9.0.0")); // incompatible
    invalid.insert(QStringLiteral("maxCoreApi"), QStringLiteral("9.0.0"));
    QVERIFY(!writeManifest(dir.path(), QStringLiteral("broken.json"), invalid).isEmpty());

    ExtensionContractRegistry registry({1, 0, 0});
    ExtensionStartupLoader loader;
    loader.registerFactory(QStringLiteral("sample.workflow"), []() {
        return std::make_unique<SampleExtensionPack>();
    });
    loader.registerFactory(QStringLiteral("broken.pack"), []() {
        return std::make_unique<SampleExtensionPack>();
    });

    const ExtensionLoadResult result = loader.loadFromDirectory(dir.path(), registry);

    QCOMPARE(result.discoveredManifestCount, 2);
    QCOMPARE(result.loadedPackCount, 1);
    QVERIFY(result.hasErrors());
    QVERIFY(registry.hasManifest(QStringLiteral("sample.workflow")));
    QVERIFY(!registry.hasManifest(QStringLiteral("broken.pack")));
    QVERIFY(diagnosticsContain(result, QStringLiteral("Manifest rejected")));
}

void ExtensionStartupLoaderTests::missingFactoryProducesClearError()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "temp dir creation failed");

    QVERIFY(!writeManifest(dir.path(), QStringLiteral("sample.workflow.json"),
                           validSampleManifestObject()).isEmpty());

    ExtensionContractRegistry registry({1, 0, 0});
    ExtensionStartupLoader loader;

    const ExtensionLoadResult result = loader.loadFromDirectory(dir.path(), registry);

    QCOMPARE(result.loadedPackCount, 0);
    QVERIFY(result.hasErrors());
    QVERIFY(diagnosticsContain(result, QStringLiteral("No pack factory registered")));
}

void ExtensionStartupLoaderTests::missingDependencyProducesClearError()
{
    QTemporaryDir dir;
    QVERIFY2(dir.isValid(), "temp dir creation failed");

    QJsonObject manifest = validSampleManifestObject();
    QJsonArray deps;
    deps.append(QStringLiteral("missing.dep"));
    manifest.insert(QStringLiteral("dependencies"), deps);
    QVERIFY(!writeManifest(dir.path(), QStringLiteral("sample.workflow.json"), manifest).isEmpty());

    ExtensionContractRegistry registry({1, 0, 0});
    ExtensionStartupLoader loader;
    loader.registerFactory(QStringLiteral("sample.workflow"), []() {
        return std::make_unique<SampleExtensionPack>();
    });

    const ExtensionLoadResult result = loader.loadFromDirectory(dir.path(), registry);

    QCOMPARE(result.loadedPackCount, 0);
    QVERIFY(result.hasErrors());
    QVERIFY(diagnosticsContain(result, QStringLiteral("Missing dependency")));
}

QTEST_MAIN(ExtensionStartupLoaderTests)
#include "tst_ExtensionStartupLoader.moc"
