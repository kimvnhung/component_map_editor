// component_map_editor/adapters/ValidationAdapter.cpp

#include "ValidationAdapter.h"

namespace cme::adapter {

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

ConversionError variantMapToValidationIssue(
    const QVariantMap &variant,
    cme::ValidationIssue &proto_out)
{
    if (!variant.contains("code"))
        return ConversionError("Missing required field: code");

    proto_out.set_code(variant["code"].toString().toStdString());

    // severity: default to ERROR if missing
    QString severity = variant.value("severity", "error").toString().toLower();
    if (severity == "error") {
        proto_out.set_severity(cme::VALIDATION_SEVERITY_ERROR);
    } else if (severity == "warning") {
        proto_out.set_severity(cme::VALIDATION_SEVERITY_WARNING);
    } else if (severity == "info") {
        proto_out.set_severity(cme::VALIDATION_SEVERITY_INFO);
    } else {
        return ConversionError("Invalid severity value: " + severity);
    }

    if (variant.contains("message"))
        proto_out.set_message(variant["message"].toString().toStdString());

    if (variant.contains("componentId"))
        proto_out.set_component_id(variant["componentId"].toString().toStdString());

    if (variant.contains("connectionId"))
        proto_out.set_connection_id(variant["connectionId"].toString().toStdString());

    return ConversionError();
}

ConversionError variantMapToGraphValidationResult(
    const QVariantMap &variant,
    cme::GraphValidationResult &proto_out)
{
    if (variant.contains("isValid")) {
        proto_out.set_is_valid(variant["isValid"].toBool());
    }

    if (variant.contains("issues")) {
        const auto issues = variant["issues"].toList();
        for (const auto &issue_variant : issues) {
            auto *pb_issue = proto_out.add_issues();
            auto err = variantMapToValidationIssue(issue_variant.toMap(), *pb_issue);
            if (err.has_error)
                return err;
        }
    }

    return ConversionError();
}

// ── Protobuf -> QVariantMap ────────────────────────────────────────────────

QVariantMap validationIssueToVariantMap(
    const cme::ValidationIssue &proto)
{
    QVariantMap result;
    result["code"] = QString::fromStdString(proto.code());

    // Severity enum -> string
    const char *severity_str = "error"; // default
    if (proto.severity() == cme::VALIDATION_SEVERITY_WARNING)
        severity_str = "warning";
    else if (proto.severity() == cme::VALIDATION_SEVERITY_INFO)
        severity_str = "info";
    result["severity"] = QString::fromUtf8(severity_str);

    if (!proto.message().empty())
        result["message"] = QString::fromStdString(proto.message());

    if (!proto.component_id().empty())
        result["componentId"] = QString::fromStdString(proto.component_id());

    if (!proto.connection_id().empty())
        result["connectionId"] = QString::fromStdString(proto.connection_id());

    return result;
}

QVariantMap graphValidationResultToVariantMap(
    const cme::GraphValidationResult &proto)
{
    QVariantMap result;
    result["isValid"] = proto.is_valid();

    QVariantList issues_list;
    for (int i = 0; i < proto.issues_size(); ++i) {
        issues_list.append(validationIssueToVariantMap(proto.issues(i)));
    }
    result["issues"] = issues_list;

    return result;
}

} // namespace cme::adapter
