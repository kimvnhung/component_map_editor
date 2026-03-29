#include "ValidationProviderV1ToV2Adapter.h"

#include "adapters/ValidationAdapter.h"

ValidationProviderV1ToV2Adapter::ValidationProviderV1ToV2Adapter(
    const IValidationProvider *legacyProvider)
    : m_legacyProvider(legacyProvider)
{
}

QString ValidationProviderV1ToV2Adapter::providerId() const
{
    if (!m_legacyProvider)
        return {};
    return m_legacyProvider->providerId();
}

bool ValidationProviderV1ToV2Adapter::validateGraph(const cme::GraphSnapshot &graphSnapshot,
                                                    cme::GraphValidationResult *outResult,
                                                    QString *error) const
{
    if (!m_legacyProvider) {
        if (error)
            *error = QStringLiteral("Legacy validation provider is null");
        return false;
    }

    if (!outResult) {
        if (error)
            *error = QStringLiteral("outResult pointer is null");
        return false;
    }

    outResult->Clear();

    const QVariantMap snapshotMap = cme::adapter::graphSnapshotForValidationToVariantMap(graphSnapshot);
    const QVariantList legacyIssues = m_legacyProvider->validateGraph(snapshotMap);

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
                *error = QStringLiteral("Failed to convert legacy issue from provider '%1': %2")
                             .arg(providerId(), conversionErr.error_message);
            }
            return false;
        }

        if (issueProto.severity() == cme::VALIDATION_SEVERITY_ERROR ||
            issueProto.severity() == cme::VALIDATION_SEVERITY_UNSPECIFIED) {
            hasErrorSeverity = true;
        }

        *outResult->add_issues() = issueProto;
    }

    outResult->set_is_valid(!hasErrorSeverity);
    return true;
}
