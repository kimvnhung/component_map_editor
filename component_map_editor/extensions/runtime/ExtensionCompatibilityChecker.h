#ifndef EXTENSIONCOMPATIBILITYCHECKER_H
#define EXTENSIONCOMPATIBILITYCHECKER_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "extensions/contracts/ExtensionApiVersion.h"
#include "extensions/contracts/ExtensionManifest.h"

// Typed severity for API change items (Phase 8: replaces raw "breaking"/"deprecated" strings).
// String literals are confined to the analyzeManifest output map ("breaking"/"deprecated" keys)
// which is a QML-facing boundary and must remain as-is for backward compatibility.
enum class ApiChangeSeverity {
    Breaking,
    Deprecated
};

struct ApiChangeItem
{
    QString id;
    QString contract;
    ApiChangeSeverity severity; // was: QString "breaking" | "deprecated"
    int minContractVersion = 1;
    QString description;
    QString migration;
};

class ExtensionCompatibilityChecker
{
public:
    explicit ExtensionCompatibilityChecker(const ExtensionApiVersion &coreApiVersion);

    QVariantMap analyzeManifest(const ExtensionManifest &manifest) const;
    bool analyzeManifestFile(const QString &manifestPath,
                             QVariantMap *report,
                             QString *error = nullptr) const;

    static QList<ApiChangeItem> catalog();
    static QStringList intentionalBreakingChangeIds();

private:
    int contractVersionFromManifest(const ExtensionManifest &manifest,
                                    const QString &contract,
                                    int defaultVersion) const;

    ExtensionApiVersion m_coreApiVersion;
};

#endif // EXTENSIONCOMPATIBILITYCHECKER_H
