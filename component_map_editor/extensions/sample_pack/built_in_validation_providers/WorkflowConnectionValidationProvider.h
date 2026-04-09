#ifndef WORKFLOWCONNECTIONVALIDATIONPROVIDER_H
#define WORKFLOWCONNECTIONVALIDATIONPROVIDER_H

#include "extensions/contracts/IValidationProvider.h"

class WorkflowConnectionValidationProvider : public IValidationProvider
{
public:
    QString      providerId() const override;
    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override;
};

#endif // WORKFLOWCONNECTIONVALIDATIONPROVIDER_H
