#include "WorkflowValidationProvider.h"

QString WorkflowValidationProvider::providerId() const
{
    return QStringLiteral("sample.workflow.validation");
}

QVariantList WorkflowValidationProvider::validateGraph(const QVariantMap &graphSnapshot) const
{
    QVariantList issues;

    const QVariantList structureIssues = m_structureProvider.validateGraph(graphSnapshot);
    for (const QVariant &issueValue : structureIssues)
        issues.append(issueValue);

    const QVariantList connectionIssues = m_connectionProvider.validateGraph(graphSnapshot);
    for (const QVariant &issueValue : connectionIssues)
        issues.append(issueValue);

    const QVariantList topologyIssues = m_topologyProvider.validateGraph(graphSnapshot);
    for (const QVariant &issueValue : topologyIssues)
        issues.append(issueValue);

    return issues;
}
