#include <QFile>
#include <QTest>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IValidationProvider.h"
#include "extensions/contracts/ExtensionApiVersion.h"

#include "graph.pb.h"
#include "validation.pb.h"

namespace {

class MapBackedValidationProviderForPhase10 : public IValidationProvider
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

class TypedValidationProviderForPhase10 : public IValidationProvider
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
    void registry_acceptsMapBackedAndTypedProvidersUnderOneCanonicalName();
    void typedFirst_validationService_noLegacySnapshotMapBridgeInsideCoreLoop();
    void contracts_useCanonicalNameWithoutDeprecatedMarkers();
};

void tst_Phase10CutoverLegacyRetirement::registry_acceptsMapBackedAndTypedProvidersUnderOneCanonicalName()
{
    ExtensionContractRegistry registry(ExtensionApiVersion{1, 0, 0});

    MapBackedValidationProviderForPhase10 mapBacked;
    TypedValidationProviderForPhase10 typed;

    QVERIFY(registry.registerValidationProvider(&mapBacked));
    QVERIFY(registry.registerValidationProvider(&typed));

    QCOMPARE(registry.validationProviders().size(), 2);
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

void tst_Phase10CutoverLegacyRetirement::contracts_useCanonicalNameWithoutDeprecatedMarkers()
{
    const QString iValidationProviderPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/component_map_editor/extensions/contracts/IValidationProvider.h");
    const QString registryPath =
        QStringLiteral("/home/hungkv/projects/component_map_editor/component_map_editor/extensions/contracts/ExtensionContractRegistry.h");

    const QString v1Contract = readTextFile(iValidationProviderPath);
    const QString registryHeader = readTextFile(registryPath);

    QVERIFY2(!v1Contract.isEmpty(), "Could not read IValidationProvider.h");
    QVERIFY2(!registryHeader.isEmpty(), "Could not read ExtensionContractRegistry.h");

    QVERIFY2(v1Contract.contains(QStringLiteral("validateGraph(const cme::GraphSnapshot")),
             "IValidationProvider.h should expose typed canonical contract");

    QVERIFY2(!v1Contract.contains(QStringLiteral("deprecated")),
             "IValidationProvider.h should not contain deprecated markers after cutover");

    QVERIFY2(!registryHeader.contains(QStringLiteral("validationProvidersV2")),
             "Registry should not expose V2-suffixed API names after canonical-name cutover");

    QVERIFY2(!registryHeader.contains(QStringLiteral("deprecated")),
             "ExtensionContractRegistry.h should not contain deprecated markers after cutover");
}

QTEST_MAIN(tst_Phase10CutoverLegacyRetirement)
#include "tst_Phase10CutoverLegacyRetirement.moc"
