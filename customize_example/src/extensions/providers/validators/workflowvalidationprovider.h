#ifndef WORKFLOWVALIDATIONPROVIDER_H
#define WORKFLOWVALIDATIONPROVIDER_H

#include <extensions/contracts/IValidationProvider.h>

class WorkflowValidationProvider : public IValidationProvider
{
public:
    QString providerId() const override;
    bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                       cme::GraphValidationResult *outResult,
                       QString *error) const override;
};

#endif // WORKFLOWVALIDATIONPROVIDER_H
