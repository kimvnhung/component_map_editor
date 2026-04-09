#ifndef CUSTOMIZEVALIDATIONPROVIDER_H
#define CUSTOMIZEVALIDATIONPROVIDER_H

#include "validators/workflowvalidationprovider.h"

class CustomizeValidationProvider : public IValidationProvider
{
public:
    QString providerId() const override;
    bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                       cme::GraphValidationResult *outResult,
                       QString *error) const override;
private:
    WorkflowValidationProvider m_workflowValidationProvider;
};

#endif // CUSTOMIZEVALIDATIONPROVIDER_H
