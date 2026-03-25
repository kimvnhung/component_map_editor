#ifndef ICONNECTIONPOLICYPROVIDER_H
#define ICONNECTIONPOLICYPROVIDER_H

#include <QString>
#include <QtPlugin>
#include <QVariantMap>

class IConnectionPolicyProvider
{
public:
    virtual ~IConnectionPolicyProvider() = default;

    virtual QString providerId() const = 0;

    // Returns true when source->target is allowed by business rules.
    virtual bool canConnect(const QString &sourceTypeId,
                            const QString &targetTypeId,
                            const QVariantMap &context,
                            QString *reason) const = 0;

    // Allows providers to normalize connection attributes before command creation.
    virtual QVariantMap normalizeConnectionProperties(const QString &sourceTypeId,
                                                      const QString &targetTypeId,
                                                      const QVariantMap &rawProperties) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_CONNECTION_POLICY_PROVIDER "ComponentMapEditor.Extensions.IConnectionPolicyProvider/1.0"
Q_DECLARE_INTERFACE(IConnectionPolicyProvider, COMPONENT_MAP_EDITOR_IID_CONNECTION_POLICY_PROVIDER)

#endif // ICONNECTIONPOLICYPROVIDER_H
