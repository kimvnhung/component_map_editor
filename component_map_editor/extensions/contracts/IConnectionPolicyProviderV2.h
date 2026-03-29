#ifndef ICONNECTIONPOLICYPROVIDERV2_H
#define ICONNECTIONPOLICYPROVIDERV2_H

#include <QString>
#include <QtPlugin>
#include <QVariantMap>

#include "policy.pb.h"

class IConnectionPolicyProviderV2
{
public:
    virtual ~IConnectionPolicyProviderV2() = default;

    virtual QString providerId() const = 0;

    // Typed contract for policy decisioning.
    virtual bool canConnect(const cme::ConnectionPolicyContext &context,
                            QString *reason) const = 0;

    // Property normalization still accepts QVariantMap for compatibility while
    // the public contract migration is in progress.
    virtual QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                                      const QVariantMap &rawProperties) const
    {
        Q_UNUSED(context)
        return rawProperties;
    }
};

#define COMPONENT_MAP_EDITOR_IID_CONNECTION_POLICY_PROVIDER_V2 "ComponentMapEditor.Extensions.IConnectionPolicyProvider/2.0"
Q_DECLARE_INTERFACE(IConnectionPolicyProviderV2, COMPONENT_MAP_EDITOR_IID_CONNECTION_POLICY_PROVIDER_V2)

#endif // ICONNECTIONPOLICYPROVIDERV2_H
