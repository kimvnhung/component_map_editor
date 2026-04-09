#ifndef SAMPLECONNECTIONPOLICYPROVIDER_H
#define SAMPLECONNECTIONPOLICYPROVIDER_H

#include "extensions/contracts/IConnectionPolicyProvider.h"

// Sample implementation of IConnectionPolicyProvider for the workflow domain.
// Allowed connections between component types:
//   start    -> process
//   process  -> process, stop
// Denied:
//   * -> start     (start has no incoming connections)
//   stop -> *       (stop has no outgoing connections)
//   start -> stop, process -> start
// normalizeConnectionProperties always adds type="flow" to the returned map.
class SampleConnectionPolicyProvider : public IConnectionPolicyProvider
{
public:
    QString     providerId() const override;
    bool        canConnect(const cme::ConnectionPolicyContext &context,
                           QString *reason) const override;
    QVariantMap normalizeConnectionProperties(const cme::ConnectionPolicyContext &context,
                                              const QVariantMap &rawProperties) const override;
};

#endif // SAMPLECONNECTIONPOLICYPROVIDER_H
