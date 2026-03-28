#ifndef CUSTOMIZECONNECTIONPOLICYPROVIDER_H
#define CUSTOMIZECONNECTIONPOLICYPROVIDER_H

#include <extensions/contracts/IConnectionPolicyProvider.h>


// Customize implementation of IConnectionPolicyProvider for the workflow domain.
// Allowed connections between component types:
//   start    -> process
//   process  -> process, stop
// Denied:
//   * -> start     (start has no incoming connections)
//   stop -> *       (stop has no outgoing connections)
//   start -> stop, process -> start
// normalizeConnectionProperties always adds type="flow" to the returned map.
class CustomizeConnectionPolicyProvider : public IConnectionPolicyProvider
{
    // IConnectionPolicyProvider interface
public:
    QString providerId() const;
    bool canConnect(const QString &sourceTypeId, const QString &targetTypeId, const QVariantMap &context, QString *reason) const;
    QVariantMap normalizeConnectionProperties(const QString &sourceTypeId, const QString &targetTypeId, const QVariantMap &rawProperties) const;
};

#endif // CUSTOMIZECONNECTIONPOLICYPROVIDER_H
