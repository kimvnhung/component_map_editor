#ifndef RULEBACKEDPROVIDERS_H
#define RULEBACKEDPROVIDERS_H

#include "extensions/contracts/IConnectionPolicyProviderV2.h"
#include "extensions/contracts/IValidationProvider.h"
#include "RuleRuntimeEngine.h"
#include "RuleRuntimeRegistry.h"

class RuleBackedConnectionPolicyProvider : public IConnectionPolicyProviderV2
{
public:
    explicit RuleBackedConnectionPolicyProvider(RuleRuntimeRegistry *registry)
        : m_registry(registry)
    {}

    QString providerId() const override
    {
        return QStringLiteral("compiled.rules.connectionPolicy");
    }

    bool canConnect(const cme::ConnectionPolicyContext &context,
                    QString *reason) const override
    {
        if (!m_registry)
            return true;

        const QString sourceTypeId = QString::fromStdString(context.source_type_id());
        const QString targetTypeId = QString::fromStdString(context.target_type_id());

        RuleRuntimeEngine engine;
        engine.setDescriptor(&m_registry->descriptor());
        return engine.canConnect(sourceTypeId, targetTypeId, reason);
    }

    QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                              const QVariantMap &rawProperties) const override
    {
        Q_UNUSED(context)

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

private:
    RuleRuntimeRegistry *m_registry = nullptr;
};

class RuleBackedValidationProvider : public IValidationProvider
{
public:
    explicit RuleBackedValidationProvider(RuleRuntimeRegistry *registry)
        : m_registry(registry)
    {}

    QString providerId() const override
    {
        return QStringLiteral("compiled.rules.validation");
    }

    bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                       cme::GraphValidationResult *outResult,
                       QString *error) const override
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

private:
    RuleRuntimeRegistry *m_registry = nullptr;
};

#endif // RULEBACKEDPROVIDERS_H
