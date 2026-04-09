#ifndef WORKFLOWVALIDATIONPROVIDER_H
#define WORKFLOWVALIDATIONPROVIDER_H

#include "WorkflowConnectionValidationProvider.h"
#include "WorkflowStructureValidationProvider.h"
#include "WorkflowTopologyValidationProvider.h"

// Built-in composite validation provider for the sample workflow domain.
// It combines specialized providers for finer reuse:
// - WorkflowStructureValidationProvider
// - WorkflowConnectionValidationProvider
// - WorkflowTopologyValidationProvider
class WorkflowValidationProvider : public IValidationProvider
{
public:
    QString      providerId() const override;
    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override;

private:
    WorkflowStructureValidationProvider  m_structureProvider;
    WorkflowConnectionValidationProvider m_connectionProvider;
    WorkflowTopologyValidationProvider   m_topologyProvider;
};

#endif // WORKFLOWVALIDATIONPROVIDER_H
