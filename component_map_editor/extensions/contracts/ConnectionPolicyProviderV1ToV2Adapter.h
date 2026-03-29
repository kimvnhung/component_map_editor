#ifndef CONNECTIONPOLICYPROVIDERV1TOV2ADAPTER_H
#define CONNECTIONPOLICYPROVIDERV1TOV2ADAPTER_H

#include "IConnectionPolicyProvider.h"
#include "IConnectionPolicyProviderV2.h"

class ConnectionPolicyProviderV1ToV2Adapter : public IConnectionPolicyProviderV2
{
public:
    explicit ConnectionPolicyProviderV1ToV2Adapter(const IConnectionPolicyProvider *legacyProvider);

    QString providerId() const override;

    bool canConnect(const cme::ConnectionPolicyContext &context,
                    QString *reason) const override;

    QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                              const QVariantMap &rawProperties) const override;

private:
    const IConnectionPolicyProvider *m_legacyProvider = nullptr;
};

#endif // CONNECTIONPOLICYPROVIDERV1TOV2ADAPTER_H
