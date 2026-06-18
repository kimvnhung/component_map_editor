#include "RuleBackedProviders.h"

#include "adapters/ValidationAdapter.h"

RuleBackedConnectionPolicyProvider::RuleBackedConnectionPolicyProvider(RuleRuntimeRegistry *registry)
    : m_registry(registry)
{
}

QString RuleBackedConnectionPolicyProvider::providerId() const
{
    return QStringLiteral("compiled.rules.connectionPolicy");
}

bool RuleBackedConnectionPolicyProvider::canConnect(const cme::ConnectionPolicyContext &context,
                                                    QString *reason) const
{
    if (!m_registry)
        return true;

    const QString sourceTypeId = QString::fromStdString(context.source_type_id());
    const QString targetTypeId = QString::fromStdString(context.target_type_id());

    RuleRuntimeEngine engine;
    engine.setDescriptor(&m_registry->descriptor());
    return engine.canConnect(sourceTypeId, targetTypeId, reason);
}

QVariantMap RuleBackedConnectionPolicyProvider::normalizeConnectionProperties(
    const cme::ConnectionPolicyContext &context,
    const QVariantMap &rawProperties) const
{
    (void)context;

    if (!m_registry)
        return rawProperties;

    QString connectionType = rawProperties.value(QStringLiteral("type")).toString();
    if (connectionType.isEmpty())
        connectionType = QStringLiteral("flow");

    const QString targetId = QStringLiteral("connection/%1").arg(connectionType);

    RuleRuntimeEngine engine;
    engine.setDescriptor(&m_registry->descriptor());
    return engine.normalizePropertiesForTarget(targetId, rawProperties);
}

RuleBackedValidationProvider::RuleBackedValidationProvider(RuleRuntimeRegistry *registry)
    : m_registry(registry)
{
}

QString RuleBackedValidationProvider::providerId() const
{
    return QStringLiteral("compiled.rules.validation");
}

bool RuleBackedValidationProvider::validateGraph(const cme::GraphSnapshot &graphSnapshot,
                                                 cme::GraphValidationResult *outResult,
                                                 QString *error) const
{
    if (!outResult) {
        if (error)
            *error = QStringLiteral("outResult is null");
        return false;
    }

    outResult->Clear();
    if (!m_registry) {
        outResult->set_is_valid(true);
        return true;
    }

    const QVariantMap graphSnapshotMap =
        cme::adapter::graphSnapshotForValidationToVariantMap(graphSnapshot);

    RuleRuntimeEngine engine;
    engine.setDescriptor(&m_registry->descriptor());
    const QVariantList issues = engine.validateGraph(graphSnapshotMap);

    bool hasError = false;
    for (const QVariant &issueValue : issues) {
        const QVariantMap issueMap = issueValue.toMap();
        if (issueMap.isEmpty())
            continue;

        cme::ValidationIssue issueProto;
        const cme::adapter::ConversionError conversionErr =
            cme::adapter::variantMapToValidationIssue(issueMap, issueProto);
        if (conversionErr.has_error) {
            if (error) {
                *error = QStringLiteral("Failed to convert rule issue: %1")
                             .arg(conversionErr.error_message);
            }
            return false;
        }

        if (issueProto.severity() == cme::VALIDATION_SEVERITY_ERROR
            || issueProto.severity() == cme::VALIDATION_SEVERITY_UNSPECIFIED) {
            hasError = true;
        }
        *outResult->add_issues() = issueProto;
    }

    outResult->set_is_valid(!hasError);
    return true;
}
