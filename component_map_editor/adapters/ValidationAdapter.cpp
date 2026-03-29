// component_map_editor/adapters/ValidationAdapter.cpp

#include "ValidationAdapter.h"
#include "GraphAdapter.h"

namespace cme::adapter {

// ── Helper: Restore sourceSide/targetSide from properties to top-level ──────
// When building typed snapshot, these are stored in proto properties map as strings.
// Legacy format expects them as top-level int fields, so we restore them here.
static void restoreConnectionSideFields(QVariantMap &connectionMap)
{
    if (connectionMap.contains("properties")) {
        auto props = connectionMap["properties"].toMap();
        
        // Extract and restore sourceSide as top-level int field
        if (props.contains("sourceSide")) {
            bool ok = false;
            int sideValue = props["sourceSide"].toInt(&ok);
            if (ok) {
                connectionMap["sourceSide"] = sideValue;
            }
            props.remove("sourceSide");
        }
        
        // Extract and restore targetSide as top-level int field
        if (props.contains("targetSide")) {
            bool ok = false;
            int sideValue = props["targetSide"].toInt(&ok);
            if (ok) {
                connectionMap["targetSide"] = sideValue;
            }
            props.remove("targetSide");
        }
        
        // Update properties map if modified
        if (props.isEmpty()) {
            connectionMap.remove("properties");
        } else {
            connectionMap["properties"] = props;
        }
    }
}

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

// Phase 5: Convert legacy snapshot map to typed GraphSnapshot proto
ConversionError variantMapToGraphSnapshotForValidation(
    const QVariantMap &snapshotMap,
    cme::GraphSnapshot &proto_out)
{
    // Delegate to GraphAdapter to handle the conversion
    return variantMapToGraphSnapshot(snapshotMap, proto_out);
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

// Phase 5: Convert typed GraphSnapshot proto back to legacy snapshot map
QVariantMap graphSnapshotForValidationToVariantMap(
    const cme::GraphSnapshot &proto)
{
    // First: delegate to GraphAdapter for base conversion
    QVariantMap result = graphSnapshotToVariantMap(proto);
    
    // Second: restore sourceSide/targetSide as top-level int fields in connections
    // (they were stored in properties map when building from proto, but legacy format expects them as top-level ints)
    if (result.contains("connections")) {
        QVariantList connections = result["connections"].toList();
        for (auto &connVar : connections) {
            auto connMap = connVar.toMap();
            restoreConnectionSideFields(connMap);
            connVar = connMap;
        }
        result["connections"] = connections;
    }
    
    return result;
}
} // namespace cme::adapter
