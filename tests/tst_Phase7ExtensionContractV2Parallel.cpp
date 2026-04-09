#include <QtTest/QtTest>

#include <algorithm>
#include <memory>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/IValidationProvider.h"
#include "services/ValidationService.h"
#include "models/GraphModel.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"

namespace {

class ValidationProviderV1CountRule : public IValidationProvider
{
public:
    QString providerId() const override { return QStringLiteral("phase7.v1.count"); }

    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override
    {
        const int componentCount = graphSnapshot.value(QStringLiteral("components")).toList().size();
        if (componentCount >= 2)
            return {};

        return {
            QVariantMap{
                { QStringLiteral("code"), QStringLiteral("P7_MIN_COMPONENTS") },
                { QStringLiteral("severity"), QStringLiteral("error") },
                { QStringLiteral("message"), QStringLiteral("Need at least 2 components") },
                { QStringLiteral("componentId"), QString() },
                { QStringLiteral("connectionId"), QString() }
            }
        };
    }
};

class ValidationProviderV1ConnectionRule : public IValidationProvider
{
public:
    QString providerId() const override { return QStringLiteral("phase7.v1.conn"); }

    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override
    {
        const int componentCount = graphSnapshot.value(QStringLiteral("components")).toList().size();
        const int connectionCount = graphSnapshot.value(QStringLiteral("connections")).toList().size();
        if (componentCount == 0 || connectionCount > 0)
            return {};

        return {
            QVariantMap{
                { QStringLiteral("code"), QStringLiteral("P7_NO_CONNECTIONS") },
                { QStringLiteral("severity"), QStringLiteral("warning") },
                { QStringLiteral("message"), QStringLiteral("No connections in graph") },
                { QStringLiteral("componentId"), QString() },
                { QStringLiteral("connectionId"), QString() }
            }
        };
    }
};

class ValidationProviderV2ConnectionRule : public IValidationProvider
{
public:
    QString providerId() const override { return QStringLiteral("phase7.v2.conn"); }

    bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                       cme::GraphValidationResult *outResult,
                       QString *error) const override
    {
        Q_UNUSED(error)
        if (!outResult)
            return false;

        outResult->Clear();
        const int componentCount = graphSnapshot.components_size();
        const int connectionCount = graphSnapshot.connections_size();
        if (componentCount > 0 && connectionCount == 0) {
            cme::ValidationIssue *issue = outResult->add_issues();
            issue->set_code("P7_NO_CONNECTIONS");
            issue->set_severity(cme::VALIDATION_SEVERITY_WARNING);
            issue->set_message("No connections in graph");
        }
        outResult->set_is_valid(true);
        return true;
    }
};

static std::unique_ptr<GraphModel> makeGraphWithTwoNodesNoConnections()
{
    std::unique_ptr<GraphModel> graph(new GraphModel());

    auto *a = new ComponentModel(graph.get());
    a->setId(QStringLiteral("A"));
    a->setType(QStringLiteral("process"));
    graph->addComponent(a);

    auto *b = new ComponentModel(graph.get());
    b->setId(QStringLiteral("B"));
    b->setType(QStringLiteral("process"));
    graph->addComponent(b);

    return graph;
}

static QStringList issueSignatures(const QVariantList &issues)
{
    QStringList out;
    out.reserve(issues.size());
    for (const QVariant &v : issues) {
        const QVariantMap issue = v.toMap();
        out.append(QStringLiteral("%1|%2|%3")
                       .arg(issue.value(QStringLiteral("code")).toString(),
                            issue.value(QStringLiteral("severity")).toString(),
                            issue.value(QStringLiteral("message")).toString()));
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace

class tst_Phase7ExtensionContractV2Parallel : public QObject
{
    Q_OBJECT

private slots:
    void mixedMode_registryAcceptsV1AndV2Together();
    void mixedMode_outputsMatchLegacyBehavior();
};

void tst_Phase7ExtensionContractV2Parallel::mixedMode_registryAcceptsV1AndV2Together()
{
    ExtensionContractRegistry registry(ExtensionApiVersion{1, 0, 0});
    ValidationProviderV1CountRule v1Count;
    ValidationProviderV2ConnectionRule v2Conn;

    QString error;
    QVERIFY2(registry.registerValidationProvider(&v1Count, &error), qPrintable(error));
    QVERIFY2(registry.registerValidationProvider(&v2Conn, &error), qPrintable(error));

    QCOMPARE(registry.validationProviders().size(), 2);
}

void tst_Phase7ExtensionContractV2Parallel::mixedMode_outputsMatchLegacyBehavior()
{
    // Baseline: two V1 providers
    ExtensionContractRegistry legacyRegistry(ExtensionApiVersion{1, 0, 0});
    ValidationProviderV1CountRule v1Count;
    ValidationProviderV1ConnectionRule v1Conn;
    QVERIFY(legacyRegistry.registerValidationProvider(&v1Count));
    QVERIFY(legacyRegistry.registerValidationProvider(&v1Conn));

    ValidationService legacyService;
    legacyService.rebuildValidationFromRegistry(legacyRegistry);

    // Mixed mode: one V1 + one V2 (same logical rules as baseline)
    ExtensionContractRegistry mixedRegistry(ExtensionApiVersion{1, 0, 0});
    ValidationProviderV2ConnectionRule v2Conn;
    QVERIFY(mixedRegistry.registerValidationProvider(&v1Count));
    QVERIFY(mixedRegistry.registerValidationProvider(&v2Conn));

    ValidationService mixedService;
    mixedService.rebuildValidationFromRegistry(mixedRegistry);

    std::unique_ptr<GraphModel> graph = makeGraphWithTwoNodesNoConnections();

    const QVariantList legacyIssues = legacyService.validationIssues(graph.get());
    const QVariantList mixedIssues = mixedService.validationIssues(graph.get());

    QCOMPARE(issueSignatures(mixedIssues), issueSignatures(legacyIssues));
}

QTEST_MAIN(tst_Phase7ExtensionContractV2Parallel)
#include "tst_Phase7ExtensionContractV2Parallel.moc"
