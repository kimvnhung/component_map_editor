// component_map_editor/adapters/DeploymentValidationAdapter.h
//
// Bidirectional adapter between the runtime's DeploymentValidationIssue and the
// unified in-process QVariantMap issue shape (specs/002-normalize-validation-issue).
//
// Unified issue keys:
//   "code"        - normalized UPPER_SNAKE_CASE (via normalizeDiagnosticCode)
//   "severity"    - "error" / "warning" / "info"
//   "path"        - stable entity identifier (field_path as-is)
//   "message"     - human-readable text
//   "componentIdentity" / "componentVersion" - flattened detail keys
//   "details"     - optional nested QVariantMap<string,string>
//   "remediation" - optional suggestion string

#pragma once

#include <QString>
#include <QVariantMap>

#include <deployment.pb.h>

namespace cme::adapter {

/// DeploymentValidationIssue -> unified QVariantMap issue (T006).
QVariantMap deploymentIssueToUnified(
    const cme::runtime::v1::DeploymentValidationIssue &issue);

/// Unified QVariantMap issue -> DeploymentValidationIssue (T006, reverse).
/// Stores the raw `code` (does not re-normalize) to avoid double-normalizing;
/// severity is parsed from the string form.
cme::runtime::v1::DeploymentValidationIssue unifiedToDeploymentIssue(
    const QVariantMap &issueMap);

} // namespace cme::adapter
