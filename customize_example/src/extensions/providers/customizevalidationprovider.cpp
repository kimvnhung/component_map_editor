#include "customizevalidationprovider.h"

QString CustomizeValidationProvider::providerId() const
{
    return QStringLiteral("customize.workflow.validation");
}

QVariantList CustomizeValidationProvider::validateGraph(const QVariantMap &graphSnapshot) const
{
    QVariantList issues;

    const QVariantList structureIssues = m_workflowValidationProvider.validateGraph(graphSnapshot);
    for (const QVariant &issueValue : structureIssues)
        issues.append(issueValue);

    return issues;
}
