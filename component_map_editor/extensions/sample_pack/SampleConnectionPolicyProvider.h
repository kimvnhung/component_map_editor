#ifndef SAMPLECONNECTIONPOLICYPROVIDER_H
#define SAMPLECONNECTIONPOLICYPROVIDER_H

#include "extensions/contracts/IConnectionPolicyProvider.h"

// Sample implementation of IConnectionPolicyProvider for the workflow domain.
// Allowed connections between component types:
//   start    -> task
//   task     -> task, decision
//   decision -> task, end
// Denied:
//   * -> start     (start has no incoming connections)
//   end -> *       (end has no outgoing connections)
//   start -> decision, start -> end, decision -> decision, task -> end (direct)
// normalizeConnectionProperties always adds type="flow" to the returned map.
class SampleConnectionPolicyProvider : public IConnectionPolicyProvider
{
public:
    QString     providerId() const override;
    bool        canConnect(const QString &sourceTypeId,
                           const QString &targetTypeId,
                           const QVariantMap &context,
                           QString *reason) const override;
    QVariantMap normalizeConnectionProperties(const QString &sourceTypeId,
                                              const QString &targetTypeId,
                                              const QVariantMap &rawProperties) const override;
};

#endif // SAMPLECONNECTIONPOLICYPROVIDER_H
