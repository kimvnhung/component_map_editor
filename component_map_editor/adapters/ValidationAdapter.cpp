// component_map_editor/adapters/ValidationAdapter.cpp

#include "ValidationAdapter.h"
#include "GraphAdapter.h"

namespace cme::adapter {

// ── Helpers: Restore legacy snapshot shape from proto properties ─────────────
// The typed GraphSnapshot stores component/connection attributes in the proto
// properties map. V1 validation providers still expect the legacy flattened
// QVariantMap shape with top-level fields like type/title/label/sourceSide.

static void restoreComponentFields(QVariantMap &componentMap)
{
    if (!componentMap.contains("properties"))
        return;

    QVariantMap props = componentMap["properties"].toMap();
    for (auto it = props.constBegin(); it != props.constEnd(); ++it)
        componentMap.insert(it.key(), it.value());

    if (componentMap.contains("typeId") && !componentMap.contains("type"))
        componentMap.insert("type", componentMap.value("typeId"));

    componentMap.remove("properties");
}

static void restoreConnectionFields(QVariantMap &connectionMap)
{
    if (!connectionMap.contains("properties"))
        return;

    QVariantMap props = connectionMap["properties"].toMap();
    for (auto it = props.constBegin(); it != props.constEnd(); ++it)
        connectionMap.insert(it.key(), it.value());

    if (connectionMap.contains("sourceSide")) {
        bool ok = false;
        const int sideValue = connectionMap.value("sourceSide").toInt(&ok);
        if (ok)
            connectionMap.insert("sourceSide", sideValue);
    }

    if (connectionMap.contains("targetSide")) {
        bool ok = false;
        const int sideValue = connectionMap.value("targetSide").toInt(&ok);
        if (ok)
            connectionMap.insert("targetSide", sideValue);
    }

    connectionMap.remove("properties");
}

// ── QVariantMap -> Protobuf ────────────────────────────────────────────────

// Legacy provider-internal shape -> stable path convention (T008/T016).
// Providers emit "entityType" ("graph"/"component"/"connection") + "entityId".
QString validationPathFor(const QString &entityType, const QString &entityId)
{
    const QString trimmedType = entityType.trimmed();
    const QString trimmedId = entityId.trimmed();
    if (trimmedType == QLatin1String("component") && !trimmedId.isEmpty())
        return QStringLiteral("components/") + trimmedId;
    if (trimmedType == QLatin1String("connection") && !trimmedId.isEmpty())
        return QStringLiteral("connections/") + trimmedId;
    return QStringLiteral("graph");
}

QString entityIdFromValidationPath(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.startsWith(QLatin1String("components/")))
        return trimmed.mid(11);
    if (trimmed.startsWith(QLatin1String("connections/")))
        return trimmed.mid(12);
    return {};
}

// T005: Read the optional "path" key and map it back onto the legacy
// component_id / connection_id fields (wire-compatible), without renumbering.
static void applyPathToIssue(const QVariantMap &variant, cme::ValidationIssue &proto_out)
{
    if (!variant.contains("path"))
        return;

    const QString path = variant["path"].toString().trimmed();
    if (path.startsWith(QLatin1String("components/")))
        proto_out.set_component_id(path.mid(QStringLiteral("components/").size()).toStdString());
    else if (path.startsWith(QLatin1String("connections/")))
        proto_out.set_connection_id(path.mid(QStringLiteral("connections/").size()).toStdString());
    // "graph" (or anything else) leaves both ids empty.
}

ConversionError variantMapToValidationIssue(
    const QVariantMap &variant,
    cme::ValidationIssue &proto_out)
{
    if (!variant.contains("code"))
        return ConversionError("Missing required field: code");

    // Normalize the code so editor and runtime emit the same stable form.
    proto_out.set_code(
        cme::editor::normalizeDiagnosticCode(variant["code"].toString()).toStdString());

    // severity: default to ERROR if missing/invalid (fail-closed)
    QString severity = variant.value("severity", "error").toString().trimmed().toLower();
    if (severity == "error" || severity.isEmpty()) {
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

    // T005: stable "path" key overrides the legacy ids when present.
    applyPathToIssue(variant, proto_out);

    // T015: optional structured context, preserved across the boundary.
    if (variant.contains("details"))
        for (auto it = variant["details"].toMap().constBegin();
             it != variant["details"].toMap().constEnd(); ++it)
            proto_out.mutable_details()->insert({it.key().toStdString(), it.value().toString().toStdString()});

    if (variant.contains("remediation"))
        proto_out.set_remediation(variant["remediation"].toString().toStdString());

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

// T004: Emit the unified shape with a stable "path" plus the legacy
// componentId/connectionId keys for backward compatibility.
QVariantMap validationIssueToVariantMap(
    const cme::ValidationIssue &proto)
{
    QVariantMap result;

    // code is already expected to be normalized; keep it as-is.
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

    // Backward-compat legacy keys.
    if (!proto.component_id().empty())
        result["componentId"] = QString::fromStdString(proto.component_id());

    if (!proto.connection_id().empty())
        result["connectionId"] = QString::fromStdString(proto.connection_id());

    // T004: stable path — prefer the proto field when set, otherwise derive it
    // from the legacy component_id / connection_id.
    QString path = QString::fromStdString(proto.path());
    if (path.isEmpty()) {
        if (!proto.component_id().empty())
            path = QStringLiteral("components/") + QString::fromStdString(proto.component_id());
        else if (!proto.connection_id().empty())
            path = QStringLiteral("connections/") + QString::fromStdString(proto.connection_id());
        else
            path = QStringLiteral("graph");
    }
    result["path"] = path;

    // T015: forward optional structured context when present.
    if (proto.details_size() > 0) {
        QVariantMap details;
        for (const auto &entry : proto.details())
            details.insert(QString::fromStdString(entry.first),
                           QString::fromStdString(entry.second));
        result["details"] = details;
    }
    if (!proto.remediation().empty())
        result["remediation"] = QString::fromStdString(proto.remediation());

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

    if (result.contains("components")) {
        QVariantList components = result["components"].toList();
        for (QVariant &componentVar : components) {
            QVariantMap componentMap = componentVar.toMap();
            restoreComponentFields(componentMap);
            componentVar = componentMap;
        }
        result["components"] = components;
    }

    if (result.contains("connections")) {
        QVariantList connections = result["connections"].toList();
        for (QVariant &connVar : connections) {
            QVariantMap connMap = connVar.toMap();
            restoreConnectionFields(connMap);
            connVar = connMap;
        }
        result["connections"] = connections;
    }

    return result;
}

// ── Severity helpers (Phase 8) ────────────────────────────────────────────

QString validationSeverityToString(cme::ValidationSeverity severity)
{
    switch (severity) {
    case cme::VALIDATION_SEVERITY_WARNING: return QStringLiteral("warning");
    case cme::VALIDATION_SEVERITY_INFO:    return QStringLiteral("info");
    default:                               return QStringLiteral("error");
    }
}

} // namespace cme::adapter
