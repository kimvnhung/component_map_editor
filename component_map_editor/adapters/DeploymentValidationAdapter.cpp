// component_map_editor/adapters/DeploymentValidationAdapter.cpp

#include "DeploymentValidationAdapter.h"
#include "ValidationAdapter.h"

namespace cme::adapter {

namespace {

QString deploymentSeverityToString(cme::runtime::v1::DeploymentValidationSeverity severity)
{
    switch (severity) {
    case cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_WARNING:
        return QStringLiteral("warning");
    case cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_UNSPECIFIED:
    default:
        // Fail-closed: unspecified severity is treated as an error.
        return QStringLiteral("error");
    }
}

cme::runtime::v1::DeploymentValidationSeverity severityFromString(const QString &severity)
{
    const QString value = severity.trimmed().toLower();
    if (value == QLatin1String("warning"))
        return cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_WARNING;
    // Fail-closed: anything else (including missing) maps to ERROR.
    return cme::runtime::v1::DEPLOYMENT_VALIDATION_SEVERITY_ERROR;
}

} // namespace

QVariantMap deploymentIssueToUnified(
    const cme::runtime::v1::DeploymentValidationIssue &issue)
{
    QVariantMap result;

    result["code"] = cme::editor::normalizeDiagnosticCode(QString::fromStdString(issue.code()));
    result["severity"] = deploymentSeverityToString(issue.severity());
    // field_path already carries stable entity identifiers; keep it as the path.
    result["path"] = QString::fromStdString(issue.field_path());
    result["message"] = QString::fromStdString(issue.message());

    // Flatten component_identity / component_version into the map so existing
    // consumers keep working, and also mirror them into a structured "details"
    // map (T015).
    if (!issue.component_identity().empty())
        result["componentIdentity"] = QString::fromStdString(issue.component_identity());
    if (!issue.component_version().empty())
        result["componentVersion"] = QString::fromStdString(issue.component_version());

    QVariantMap details;
    if (!issue.component_identity().empty())
        details["componentIdentity"] = QString::fromStdString(issue.component_identity());
    if (!issue.component_version().empty())
        details["componentVersion"] = QString::fromStdString(issue.component_version());
    if (!details.isEmpty())
        result["details"] = details;

    return result;
}

cme::runtime::v1::DeploymentValidationIssue unifiedToDeploymentIssue(
    const QVariantMap &issueMap)
{
    cme::runtime::v1::DeploymentValidationIssue issue;

    // Store the raw code as provided so we do not double-normalize; the
    // caller is expected to pass either an already-normalized or a raw code.
    issue.set_code(issueMap.value("code").toString().toStdString());
    issue.set_severity(severityFromString(issueMap.value("severity").toString()));
    issue.set_field_path(issueMap.value("path").toString().toStdString());
    issue.set_message(issueMap.value("message").toString().toStdString());

    // component_identity / component_version: prefer flattened keys, falling
    // back to the nested "details" map when present.
    const QVariantMap details = issueMap.value("details").toMap();
    const QString identity = issueMap.contains("componentIdentity")
        ? issueMap.value("componentIdentity").toString()
        : details.value("componentIdentity").toString();
    const QString version = issueMap.contains("componentVersion")
        ? issueMap.value("componentVersion").toString()
        : details.value("componentVersion").toString();

    if (!identity.isEmpty())
        issue.set_component_identity(identity.toStdString());
    if (!version.isEmpty())
        issue.set_component_version(version.toStdString());

    return issue;
}

} // namespace cme::adapter
