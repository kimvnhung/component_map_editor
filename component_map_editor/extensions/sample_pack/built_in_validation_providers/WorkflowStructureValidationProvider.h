#ifndef WORKFLOWSTRUCTUREVALIDATIONPROVIDER_H
#define WORKFLOWSTRUCTUREVALIDATIONPROVIDER_H

#include "extensions/contracts/IValidationProvider.h"

class WorkflowStructureValidationProvider : public IValidationProvider
{
public:
    QString      providerId() const override;
    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override;
};

#endif // WORKFLOWSTRUCTUREVALIDATIONPROVIDER_H
