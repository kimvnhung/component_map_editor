#ifndef WORKFLOWTOPOLOGYVALIDATIONPROVIDER_H
#define WORKFLOWTOPOLOGYVALIDATIONPROVIDER_H

#include "extensions/contracts/IValidationProvider.h"

class WorkflowTopologyValidationProvider : public IValidationProvider
{
public:
    QString      providerId() const override;
    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override;
};

#endif // WORKFLOWTOPOLOGYVALIDATIONPROVIDER_H
