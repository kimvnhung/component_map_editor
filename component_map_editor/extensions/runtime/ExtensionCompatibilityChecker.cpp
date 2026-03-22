#include "ExtensionCompatibilityChecker.h"

#include <QJsonDocument>
#include <QJsonObject>

#include "extensions/runtime/ExtensionManifestJson.h"

namespace {

QVariantMap changeToVariant(const ApiChangeItem &item)
{
    QVariantMap v;
    v.insert(QStringLiteral("id"), item.id);
    v.insert(QStringLiteral("contract"), item.contract);
    v.insert(QStringLiteral("severity"), item.severity);
    v.insert(QStringLiteral("minContractVersion"), item.minContractVersion);
    v.insert(QStringLiteral("description"), item.description);
    v.insert(QStringLiteral("migration"), item.migration);
    return v;
}

} // namespace

ExtensionCompatibilityChecker::ExtensionCompatibilityChecker(const ExtensionApiVersion &coreApiVersion)
    : m_coreApiVersion(coreApiVersion)
{}

QList<ApiChangeItem> ExtensionCompatibilityChecker::catalog()
{
    return {
        {
            QStringLiteral("EXT-EXEC-001"),
            QStringLiteral("executionSemantics"),
            QStringLiteral("breaking"),
            1,
            QStringLiteral("Execution semantics contract v1 introduces trace output requirements."),
            QStringLiteral("Migrate provider to v1 signature or rely on core v0 adapter; ensure trace fields are emitted.")
        },
        {
            QStringLiteral("EXT-ACTION-DEP-001"),
            QStringLiteral("actions"),
            QStringLiteral("deprecated"),
            1,
            QStringLiteral("Action descriptor key 'iconName' is deprecated; use 'icon'."),
            QStringLiteral("Update action descriptors to provide 'icon' and keep 'iconName' only during transition.")
        },
        {
            QStringLiteral("EXT-SCHEMA-DEP-001"),
            QStringLiteral("propertySchema"),
            QStringLiteral("deprecated"),
            1,
            QStringLiteral("Property schema key 'widget' is deprecated; use 'editor'."),
            QStringLiteral("Rename schema field 'widget' to 'editor' across schemas.")
        }
    };
}

QStringList ExtensionCompatibilityChecker::intentionalBreakingChangeIds()
{
    QStringList ids;
    for (const ApiChangeItem &item : catalog()) {
        if (item.severity == QStringLiteral("breaking"))
            ids.append(item.id);
    }
    return ids;
}

int ExtensionCompatibilityChecker::contractVersionFromManifest(const ExtensionManifest &manifest,
                                                               const QString &contract,
                                                               int defaultVersion) const
{
    const QVariantMap metadata = manifest.metadata;
    const QVariantMap contractVersions = metadata.value(QStringLiteral("contractVersions")).toMap();
    if (!contractVersions.contains(contract))
        return defaultVersion;
    bool ok = false;
    const int version = contractVersions.value(contract).toInt(&ok);
    if (!ok)
        return defaultVersion;
    return version;
}

QVariantMap ExtensionCompatibilityChecker::analyzeManifest(const ExtensionManifest &manifest) const
{
    QVariantMap report;
    QVariantList breakings;
    QVariantList deprecations;

    ExtensionCompatibilityReport apiRangeReport = evaluateCompatibility(
        m_coreApiVersion, manifest.minCoreApi, manifest.maxCoreApi);

    for (const ApiChangeItem &item : catalog()) {
        // Only check contract entries for capabilities declared by the pack.
        if (!manifest.capabilities.contains(item.contract))
            continue;

        const int version = contractVersionFromManifest(manifest, item.contract, 1);
        if (version < item.minContractVersion) {
            if (item.severity == QStringLiteral("breaking"))
                breakings.append(changeToVariant(item));
            else
                deprecations.append(changeToVariant(item));
        }
    }

    report.insert(QStringLiteral("extensionId"), manifest.extensionId);
    report.insert(QStringLiteral("displayName"), manifest.displayName);
    report.insert(QStringLiteral("coreApi"), m_coreApiVersion.toString());
    report.insert(QStringLiteral("declaredRange"),
                  QStringLiteral("%1 - %2").arg(manifest.minCoreApi.toString(), manifest.maxCoreApi.toString()));
    report.insert(QStringLiteral("rangeCompatible"), apiRangeReport.compatible());
    report.insert(QStringLiteral("rangeMessage"), apiRangeReport.message);
    report.insert(QStringLiteral("breaking"), breakings);
    report.insert(QStringLiteral("deprecated"), deprecations);
    report.insert(QStringLiteral("breakingCount"), breakings.size());
    report.insert(QStringLiteral("deprecatedCount"), deprecations.size());
    report.insert(QStringLiteral("compatible"), apiRangeReport.compatible() && breakings.isEmpty());
    return report;
}

bool ExtensionCompatibilityChecker::analyzeManifestFile(const QString &manifestPath,
                                                        QVariantMap *report,
                                                        QString *error) const
{
    if (!report) {
        if (error)
            *error = QStringLiteral("Report output pointer is null.");
        return false;
    }

    ExtensionManifest manifest;
    QString parseError;
    if (!ExtensionManifestJson::parseFile(manifestPath, &manifest, &parseError)) {
        if (error)
            *error = parseError;
        return false;
    }

    *report = analyzeManifest(manifest);
    return true;
}
