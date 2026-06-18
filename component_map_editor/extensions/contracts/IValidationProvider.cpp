#include "IValidationProvider.h"

bool IValidationProvider::validateGraph(const cme::GraphSnapshot &graphSnapshot,
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

QVariantList IValidationProvider::validateGraph(const QVariantMap &graphSnapshot) const
{
    (void)graphSnapshot;
    return {};
}
