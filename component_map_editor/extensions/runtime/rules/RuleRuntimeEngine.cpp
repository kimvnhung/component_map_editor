#include "RuleRuntimeEngine.h"

#include <QSet>

void RuleRuntimeEngine::setDescriptor(const CompiledRuleDescriptor *descriptor)
{
    m_descriptor = descriptor;
}

bool RuleRuntimeEngine::canConnect(const QString &sourceTypeId,
                                   const QString &targetTypeId,
                                   QString *reason) const
{
    if (!m_descriptor)
        return true;

    const auto sourceIt = m_descriptor->connectionTable.constFind(sourceTypeId);
    if (sourceIt == m_descriptor->connectionTable.constEnd()) {
        if (!m_descriptor->defaultConnectionAllow && reason)
            *reason = QStringLiteral("Connection denied by default policy for source '%1'.")
                          .arg(sourceTypeId);
        return m_descriptor->defaultConnectionAllow;
    }

    const auto targetIt = sourceIt->constFind(targetTypeId);
    if (targetIt == sourceIt->constEnd()) {
        if (!m_descriptor->defaultConnectionAllow && reason)
            *reason = QStringLiteral("Connection denied by default policy for '%1' -> '%2'.")
                          .arg(sourceTypeId, targetTypeId);
        return m_descriptor->defaultConnectionAllow;
    }

    if (!targetIt->allow && reason) {
        if (!targetIt->reason.isEmpty())
            *reason = targetIt->reason;
        else
            *reason = QStringLiteral("Connection denied by compiled business rule.");
    }

    return targetIt->allow;
}

QVariantMap RuleRuntimeEngine::normalizePropertiesForTarget(const QString &targetId,
                                                            const QVariantMap &rawProperties) const
{
    QVariantMap result = rawProperties;
    if (!m_descriptor)
        return result;

    const QVariantMap derived = m_descriptor->derivedPropertyTable.value(targetId);
    for (auto it = derived.constBegin(); it != derived.constEnd(); ++it) {
        if (!result.contains(it.key()))
            result.insert(it.key(), it.value());
    }

    return result;
}

QVariantList RuleRuntimeEngine::validateGraph(const QVariantMap &graphSnapshot) const
{
    QVariantList issues;
    if (!m_descriptor)
        return issues;

    const QVariantList components = graphSnapshot.value(QStringLiteral("components")).toList();
    const QVariantList connections = graphSnapshot.value(QStringLiteral("connections")).toList();

    QSet<QString> componentIds;
    QHash<QString, int> typeCounts;
    for (const QVariant &value : components) {
        const QVariantMap component = value.toMap();
        const QString id = component.value(QStringLiteral("id")).toString();
        const QString type = component.value(QStringLiteral("type")).toString();
        if (!id.isEmpty())
            componentIds.insert(id);
        if (!type.isEmpty())
            typeCounts[type] += 1;
    }

    for (const CompiledValidationRule &rule : m_descriptor->validationRules) {
        switch (rule.kind) {
        case RuleKind::ExactlyOneType: {
            const int count = typeCounts.value(rule.type);
            if (count != 1) {
                issues.append(QVariantMap{
                    { QStringLiteral("code"), rule.code },
                    { QStringLiteral("severity"), rule.severity },
                    { QStringLiteral("message"),
                      QStringLiteral("%1 (type='%2', found=%3)")
                          .arg(rule.message, rule.type)
                          .arg(count) },
                    { QStringLiteral("entityType"), QStringLiteral("graph") },
                    { QStringLiteral("entityId"), QString() }
                });
            }
            break;
        }
        case RuleKind::EndpointExists: {
            for (const QVariant &value : connections) {
                const QVariantMap connection = value.toMap();
                const QString connectionId = connection.value(QStringLiteral("id")).toString();
                const QString sourceId = connection.value(QStringLiteral("sourceId")).toString();
                const QString targetId = connection.value(QStringLiteral("targetId")).toString();

                if (!componentIds.contains(sourceId)) {
                    issues.append(QVariantMap{
                        { QStringLiteral("code"), rule.code },
                        { QStringLiteral("severity"), rule.severity },
                        { QStringLiteral("message"),
                          QStringLiteral("%1 (connection='%2', missing source='%3')")
                              .arg(rule.message, connectionId, sourceId) },
                        { QStringLiteral("entityType"), QStringLiteral("connection") },
                        { QStringLiteral("entityId"), connectionId }
                    });
                }

                if (!componentIds.contains(targetId)) {
                    issues.append(QVariantMap{
                        { QStringLiteral("code"), rule.code },
                        { QStringLiteral("severity"), rule.severity },
                        { QStringLiteral("message"),
                          QStringLiteral("%1 (connection='%2', missing target='%3')")
                              .arg(rule.message, connectionId, targetId) },
                        { QStringLiteral("entityType"), QStringLiteral("connection") },
                        { QStringLiteral("entityId"), connectionId }
                    });
                }
            }
            break;
        }
        case RuleKind::NoIsolated: {
            QSet<QString> connectedIds;
            for (const QVariant &value : connections) {
                const QVariantMap connection = value.toMap();
                connectedIds.insert(connection.value(QStringLiteral("sourceId")).toString());
                connectedIds.insert(connection.value(QStringLiteral("targetId")).toString());
            }

            for (const QVariant &value : components) {
                const QVariantMap component = value.toMap();
                const QString id = component.value(QStringLiteral("id")).toString();
                if (!id.isEmpty() && !connectedIds.contains(id)) {
                    issues.append(QVariantMap{
                        { QStringLiteral("code"), rule.code },
                        { QStringLiteral("severity"), rule.severity },
                        { QStringLiteral("message"),
                          QStringLiteral("%1 (component='%2')").arg(rule.message, id) },
                        { QStringLiteral("entityType"), QStringLiteral("component") },
                        { QStringLiteral("entityId"), id }
                    });
                }
            }
            break;
        }
        default:
            // RuleKind::Unknown — skipped (compiler already emitted a diagnostic)
            break;
        }
    }

    return issues;
}
