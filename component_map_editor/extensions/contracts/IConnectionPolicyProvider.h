#ifndef ICONNECTIONPOLICYPROVIDER_H
#define ICONNECTIONPOLICYPROVIDER_H

#include <QString>
#include <QtPlugin>
#include <QVariantMap>

#include "policy.pb.h"

class IConnectionPolicyProvider
{
public:
    virtual ~IConnectionPolicyProvider() = default;

    virtual QString providerId() const = 0;

    // Typed contract for policy decisioning.
    virtual bool canConnect(const cme::ConnectionPolicyContext &context,
                            QString *reason) const = 0;

    // Property normalization still accepts QVariantMap for compatibility while
    // public contract migration is rolled out across call sites.
    virtual QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                                      const QVariantMap &rawProperties) const
    {
        Q_UNUSED(context)
        return rawProperties;
    }
};

#define COMPONENT_MAP_EDITOR_IID_CONNECTION_POLICY_PROVIDER "ComponentMapEditor.Extensions.IConnectionPolicyProvider/2.0"
Q_DECLARE_INTERFACE(IConnectionPolicyProvider, COMPONENT_MAP_EDITOR_IID_CONNECTION_POLICY_PROVIDER)

#endif // ICONNECTIONPOLICYPROVIDER_H
