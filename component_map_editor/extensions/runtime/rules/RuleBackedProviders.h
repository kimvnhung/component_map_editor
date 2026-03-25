#ifndef RULEBACKEDPROVIDERS_H
#define RULEBACKEDPROVIDERS_H

#include "extensions/contracts/IConnectionPolicyProvider.h"
#include "extensions/contracts/IValidationProvider.h"
#include "RuleRuntimeEngine.h"
#include "RuleRuntimeRegistry.h"

class RuleBackedConnectionPolicyProvider : public IConnectionPolicyProvider
{
public:
    explicit RuleBackedConnectionPolicyProvider(RuleRuntimeRegistry *registry)
        : m_registry(registry)
    {}

    QString providerId() const override
    {
        return QStringLiteral("compiled.rules.connectionPolicy");
    }

    bool canConnect(const QString &sourceTypeId,
                    const QString &targetTypeId,
                    const QVariantMap & /*context*/,
                    QString *reason) const override
    {
        if (!m_registry)
            return true;

        RuleRuntimeEngine engine;
        engine.setDescriptor(&m_registry->descriptor());
        return engine.canConnect(sourceTypeId, targetTypeId, reason);
    }

    QVariantMap normalizeConnectionProperties(const QString &sourceTypeId,
                                              const QString &targetTypeId,
                                              const QVariantMap &rawProperties) const override
    {
        Q_UNUSED(sourceTypeId)
        Q_UNUSED(targetTypeId)

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

    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override
    {
        if (!m_registry)
            return {};

        RuleRuntimeEngine engine;
        engine.setDescriptor(&m_registry->descriptor());
        return engine.validateGraph(graphSnapshot);
    }

private:
    RuleRuntimeRegistry *m_registry = nullptr;
};

#endif // RULEBACKEDPROVIDERS_H
