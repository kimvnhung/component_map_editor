#ifndef IVALIDATIONPROVIDER_H
#define IVALIDATIONPROVIDER_H

#include <QString>
#include <QtPlugin>
#include <QVariantList>
#include <QVariantMap>

#include "adapters/ValidationAdapter.h"
#include "graph.pb.h"
#include "validation.pb.h"

class IValidationProvider
{
public:
    virtual ~IValidationProvider() = default;

    virtual QString providerId() const = 0;

    // Canonical typed contract.
    // Default implementation auto-bridges legacy map-based providers so
    // existing providers keep working during cutover.
    virtual bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                               cme::GraphValidationResult *outResult,
                               QString *error) const
    {
        if (!outResult) {
            if (error)
                *error = QStringLiteral("outResult pointer is null");
            return false;
        }

        outResult->Clear();

        const QVariantMap snapshotMap = cme::adapter::graphSnapshotForValidationToVariantMap(graphSnapshot);
        const QVariantList legacyIssues = validateGraph(snapshotMap);

        bool hasErrorSeverity = false;
        for (const QVariant &issueValue : legacyIssues) {
            const QVariantMap issueMap = issueValue.toMap();
            if (issueMap.isEmpty())
                continue;

            cme::ValidationIssue issueProto;
            const cme::adapter::ConversionError conversionErr =
                cme::adapter::variantMapToValidationIssue(issueMap, issueProto);
            if (conversionErr.has_error) {
                if (error) {
                    *error = QStringLiteral("Failed to convert validation issue from provider '%1': %2")
                                 .arg(providerId(), conversionErr.error_message);
                }
                return false;
            }

            if (issueProto.severity() == cme::VALIDATION_SEVERITY_ERROR
                || issueProto.severity() == cme::VALIDATION_SEVERITY_UNSPECIFIED) {
                hasErrorSeverity = true;
            }

            *outResult->add_issues() = issueProto;
        }

        outResult->set_is_valid(!hasErrorSeverity);
        return true;
    }

    // Legacy map-based contract kept as optional fallback during cutover.
    virtual QVariantList validateGraph(const QVariantMap &graphSnapshot) const
    {
        Q_UNUSED(graphSnapshot)
        return {};
    }
};

#define COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER "ComponentMapEditor.Extensions.IValidationProvider/2.0"
Q_DECLARE_INTERFACE(IValidationProvider, COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER)

#endif // IVALIDATIONPROVIDER_H
