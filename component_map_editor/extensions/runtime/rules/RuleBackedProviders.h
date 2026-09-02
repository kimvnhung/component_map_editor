#ifndef RULEBACKEDPROVIDERS_H
#define RULEBACKEDPROVIDERS_H

#include "extensions/contracts/IConnectionPolicyProvider.h"
#include "extensions/contracts/IValidationProvider.h"
#include "RuleRuntimeEngine.h"
#include "RuleRuntimeRegistry.h"

class RuleBackedConnectionPolicyProvider : public IConnectionPolicyProvider
{
public:
    explicit RuleBackedConnectionPolicyProvider(RuleRuntimeRegistry *registry);

    QString providerId() const override;

    bool canConnect(const cme::ConnectionPolicyContext &context,
                    QString *reason) const override;

    QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                              const QVariantMap &rawProperties) const override;

private:
    RuleRuntimeRegistry *m_registry = nullptr;
};

class RuleBackedValidationProvider : public IValidationProvider
{
public:
    explicit RuleBackedValidationProvider(RuleRuntimeRegistry *registry);

    QString providerId() const override;

    bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                       cme::GraphValidationResult *outResult,
                       QString *error) const override;

private:
    RuleRuntimeRegistry *m_registry = nullptr;
};

#endif // RULEBACKEDPROVIDERS_H
