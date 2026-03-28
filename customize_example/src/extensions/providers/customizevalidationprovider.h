#ifndef CUSTOMIZEVALIDATIONPROVIDER_H
#define CUSTOMIZEVALIDATIONPROVIDER_H

#include "validators/workflowvalidationprovider.h"

class CustomizeValidationProvider : public IValidationProvider
{
public:
    QString providerId() const;
    QVariantList validateGraph(const QVariantMap &graphSnapshot) const;
private:
    WorkflowValidationProvider m_workflowValidationProvider;
};

#endif // CUSTOMIZEVALIDATIONPROVIDER_H
