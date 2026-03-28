#ifndef WORKFLOWVALIDATIONPROVIDER_H
#define WORKFLOWVALIDATIONPROVIDER_H

#include <extensions/contracts/IValidationProvider.h>

class WorkflowValidationProvider : public IValidationProvider
{
public:
    QString providerId() const;
    QVariantList validateGraph(const QVariantMap &graphSnapshot) const;
};

#endif // WORKFLOWVALIDATIONPROVIDER_H
