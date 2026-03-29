#include "customizevalidationprovider.h"

QString CustomizeValidationProvider::providerId() const
{
    return QStringLiteral("customize.workflow.validation");
}

bool CustomizeValidationProvider::validateGraph(const cme::GraphSnapshot &graphSnapshot,
                                                cme::GraphValidationResult *outResult,
                                                QString *error) const
{
    if (!outResult) {
        if (error)
            *error = QStringLiteral("outResult pointer is null");
        return false;
    }

    outResult->Clear();

    cme::GraphValidationResult workflowResult;
    QString workflowError;
    if (!m_workflowValidationProvider.validateGraph(graphSnapshot, &workflowResult, &workflowError)) {
        if (error)
            *error = workflowError;
        return false;
    }

    for (int i = 0; i < workflowResult.issues_size(); ++i)
        *outResult->add_issues() = workflowResult.issues(i);

    outResult->set_is_valid(workflowResult.is_valid());
    return true;
}
