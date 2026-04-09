#include "workflowvalidationprovider.h"

#include <QSet>

namespace {

QVariantMap makeIssue(const QString &code,
                      const QString &severity,
                      const QString &message,
                      const QString &entityType,
                      const QString &entityId)
{
    return QVariantMap{
        { QStringLiteral("code"), code },
        { QStringLiteral("severity"), severity },
        { QStringLiteral("message"), message },
        { QStringLiteral("entityType"), entityType },
        { QStringLiteral("entityId"), entityId }
    };
}

} // namespace

QString WorkflowValidationProvider::providerId() const
{
    return QStringLiteral("customize.workflow.validation.structure");
}

bool WorkflowValidationProvider::validateGraph(const cme::GraphSnapshot &graphSnapshot,
                                               cme::GraphValidationResult *outResult,
                                               QString *error) const
{
    if (!outResult) {
        if (error)
            *error = QStringLiteral("outResult pointer is null");
        return false;
    }

    outResult->Clear();

    const QVariantMap snapshotMap = cme::adapter::graphSnapshotForValidationToVariantMap(graphSnapshot);
    const QVariantList components = snapshotMap.value(QStringLiteral("components")).toList();
    QVariantList issues;

    QSet<QString> componentIds;
    int startCount = 0;
    int stopCount  = 0;

    for (const QVariant &v : components) {
        const QVariantMap component = v.toMap();
        const QString id = component.value(QStringLiteral("id")).toString();
        const QString type = component.value(QStringLiteral("type")).toString();
        if (id.isEmpty()) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Component has an empty id"),
                                    QStringLiteral("component"),
                                    QString()));
            continue;
        }

        if (componentIds.contains(id)) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Duplicate component id: %1").arg(id),
                                    QStringLiteral("component"),
                                    id));
        } else {
            componentIds.insert(id);
        }

        if (type == QStringLiteral("start"))
            ++startCount;
        if (type == QStringLiteral("stop"))
            ++stopCount;

        if (component.value(QStringLiteral("width")).toDouble() <= 0.0) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Component '%1' has non-positive width").arg(id),
                                    QStringLiteral("component"),
                                    id));
        }
        if (component.value(QStringLiteral("height")).toDouble() <= 0.0) {
            issues.append(makeIssue(QStringLiteral("W005"),
                                    QStringLiteral("error"),
                                    QStringLiteral("Component '%1' has non-positive height").arg(id),
                                    QStringLiteral("component"),
                                    id));
        }
    }

    if (startCount != 1) {
        issues.append(makeIssue(QStringLiteral("W001"),
                                QStringLiteral("error"),
                                QStringLiteral("Graph must contain exactly one start component (found %1).").arg(startCount),
                                QStringLiteral("graph"),
                                QString()));
    }

    if (stopCount != 1) {
        issues.append(makeIssue(QStringLiteral("W002"),
                                QStringLiteral("error"),
                                QStringLiteral("Graph must contain exactly one stop component (found %1).").arg(stopCount),
                                QStringLiteral("graph"),
                                QString()));
    }

    bool hasErrorSeverity = false;
    for (const QVariant &issueValue : issues) {
        const QVariantMap issueMap = issueValue.toMap();
        if (issueMap.isEmpty())
            continue;

        cme::ValidationIssue issueProto;
        const cme::adapter::ConversionError conversionErr =
            cme::adapter::variantMapToValidationIssue(issueMap, issueProto);
        if (conversionErr.has_error) {
            if (error) {
                *error = QStringLiteral("Failed to convert validation issue from provider '%1': %2")
                             .arg(providerId(), conversionErr.error_message);
            }
            return false;
        }

        if (issueProto.severity() == cme::VALIDATION_SEVERITY_ERROR
            || issueProto.severity() == cme::VALIDATION_SEVERITY_UNSPECIFIED) {
            hasErrorSeverity = true;
        }

        *outResult->add_issues() = issueProto;
    }

    outResult->set_is_valid(!hasErrorSeverity);
    return true;
}