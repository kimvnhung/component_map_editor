#include <QFile>
#include <QTest>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IValidationProvider.h"
#include "extensions/contracts/IValidationProviderV2.h"
#include "extensions/contracts/ExtensionApiVersion.h"

#include "graph.pb.h"
#include "validation.pb.h"

namespace {

class V1LegacyValidationProviderForPhase10 : public IValidationProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase10.v1.legacy");
    }

    QVariantList validateGraph(const QVariantMap &) const override
    {
        return {};
    }
};

class V2TypedValidationProviderForPhase10 : public IValidationProviderV2
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase10.v2.typed");
    }

    bool validateGraph(const cme::GraphSnapshot &, cme::GraphValidationResult *outResult, QString *error) const override
    {
        if (!outResult) {
            if (error)
                *error = QStringLiteral("outResult is null");
            return false;
        }
        outResult->Clear();
        outResult->set_is_valid(true);
        return true;
    }
};

QString readTextFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readAll());
}

} // namespace

class tst_Phase10CutoverLegacyRetirement : public QObject
{
    Q_OBJECT

private slots:
    void registry_monitorsLegacyV1Registration_duringCompatibilityWindow();
    void typedFirst_validationService_noLegacySnapshotMapBridgeInsideCoreLoop();
    void contracts_markV1AsDeprecated();
};

void tst_Phase10CutoverLegacyRetirement::registry_monitorsLegacyV1Registration_duringCompatibilityWindow()
{
    ExtensionContractRegistry registry(ExtensionApiVersion{1, 0, 0});

    V1LegacyValidationProviderForPhase10 v1;
    V2TypedValidationProviderForPhase10 v2;

    QVERIFY(registry.registerValidationProvider(&v1));
    QVERIFY(registry.registerValidationProvider(&v2));

    QCOMPARE(registry.legacyValidationProviderRegistrations(), 1);
    QCOMPARE(registry.validationProvidersV2().size(), 2);
    QCOMPARE(registry.validationProviders().size(), 1);
}

void tst_Phase10CutoverLegacyRetirement::typedFirst_validationService_noLegacySnapshotMapBridgeInsideCoreLoop()
{
    const QString path =
        QStringLiteral("/home/hungkv/projects/component_map_editor/component_map_editor/services/ValidationService.cpp");
    const QString src = readTextFile(path);
    QVERIFY2(!src.isEmpty(), "Could not read ValidationService.cpp");

    // Phase 10 cutover requirement: ValidationService should execute providers
    // directly from typedSnapshot, not build an intermediate legacy map snapshot.
    QVERIFY2(!src.contains(QStringLiteral("graphSnapshotForValidationToVariantMap(typedSnapshot)")),
             "Legacy snapshot map bridge is still present in ValidationService typed-first core loop");

    QVERIFY2(!src.contains(QStringLiteral("const QVariantMap snapshot =")),
             "Unexpected legacy snapshot variable found in ValidationService.cpp");
}

void tst_Phase10CutoverLegacyRetirement::contracts_markV1AsDeprecated()
{
    const QString iValidationProviderPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/component_map_editor/extensions/contracts/IValidationProvider.h");
    const QString registryPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/component_map_editor/extensions/contracts/ExtensionContractRegistry.h");

    const QString v1Contract = readTextFile(iValidationProviderPath);
    const QString registryHeader = readTextFile(registryPath);

    QVERIFY2(!v1Contract.isEmpty(), "Could not read IValidationProvider.h");
    QVERIFY2(!registryHeader.isEmpty(), "Could not read ExtensionContractRegistry.h");

    QVERIFY2(v1Contract.contains(QStringLiteral("deprecated")),
             "IValidationProvider.h should mark V1 interface/method as deprecated");

    QVERIFY2(registryHeader.contains(QStringLiteral("registerValidationProvider(IValidationProvider*) is deprecated")),
             "Registry should mark V1 registration API as deprecated");

    QVERIFY2(registryHeader.contains(QStringLiteral("legacyValidationProviderRegistrations")),
             "Registry should expose legacy registration monitoring accessor");
}

QTEST_MAIN(tst_Phase10CutoverLegacyRetirement)
#include "tst_Phase10CutoverLegacyRetirement.moc"
