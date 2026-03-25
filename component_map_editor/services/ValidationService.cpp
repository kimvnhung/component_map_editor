#include "ValidationService.h"

#include "extensions/contracts/ExtensionContractRegistry.h"

ValidationService::ValidationService(QObject *parent)
    : QObject(parent)
{}

void ValidationService::setValidationProviders(const QList<const IValidationProvider *> &providers)
{
    m_validationProviders.clear();
    for (const IValidationProvider *provider : providers) {
        if (provider)
            m_validationProviders.append(provider);
    }
}

void ValidationService::rebuildValidationFromRegistry(const ExtensionContractRegistry &registry)
{
    setValidationProviders(registry.validationProviders());
}

QStringList ValidationService::validationErrors(GraphModel *graph)
{
    QStringList errors;
    const QVariantList issues = validationIssues(graph);
    for (const QVariant &issueValue : issues) {
        const QVariantMap issue = issueValue.toMap();
        if (!issueIsError(issue))
            continue;

        const QString message = issue.value(QStringLiteral("message")).toString().trimmed();
        if (!message.isEmpty())
            errors.append(message);
    }

    return errors;
}

QVariantList ValidationService::validationIssues(GraphModel *graph)
{
    QVariantList issues;
    if (!graph) {
        issues.append(QVariantMap{
            { QStringLiteral("code"), QStringLiteral("CORE_NULL_GRAPH") },
            { QStringLiteral("severity"), QStringLiteral("error") },
            { QStringLiteral("message"), QStringLiteral("Graph is null") },
            { QStringLiteral("entityType"), QStringLiteral("graph") },
            { QStringLiteral("entityId"), QString() }
        });
        return issues;
    }

    if (m_validationProviders.isEmpty())
        return issues;

    const QVariantMap snapshot = buildGraphSnapshot(graph);
    for (const IValidationProvider *provider : std::as_const(m_validationProviders)) {
        if (!provider)
            continue;

        const QVariantList providerIssues = provider->validateGraph(snapshot);
        for (const QVariant &issueValue : providerIssues) {
            const QVariantMap issue = issueValue.toMap();
            if (issue.isEmpty())
                continue;
            issues.append(issue);
        }
    }

    return issues;
}

bool ValidationService::validate(GraphModel *graph)
{
    QStringList errors;
    const QVariantList issues = validationIssues(graph);
    for (const QVariant &issueValue : issues) {
            const QVariantMap issue = issueValue.toMap();
            if (!issueIsError(issue))
                continue;

            const QString message = issue.value(QStringLiteral("message")).toString().trimmed();
            if (!message.isEmpty()) {
                errors.append(message);
                break;
            }
    }

    return errors.isEmpty();
}

QVariantMap ValidationService::buildGraphSnapshot(GraphModel *graph) const
{
    QVariantList components;
    const QList<ComponentModel *> componentList = graph ? graph->componentList() : QList<ComponentModel *>();
    components.reserve(componentList.size());
    for (ComponentModel *component : componentList) {
        if (!component)
            continue;

        QVariantMap entry{
            { QStringLiteral("id"), component->id() },
            { QStringLiteral("title"), component->title() },
            { QStringLiteral("x"), component->x() },
            { QStringLiteral("y"), component->y() },
            { QStringLiteral("width"), component->width() },
            { QStringLiteral("height"), component->height() },
            { QStringLiteral("color"), component->color() },
            { QStringLiteral("type"), component->type() },
            { QStringLiteral("shape"), component->shape() },
            { QStringLiteral("content"), component->content() },
            { QStringLiteral("icon"), component->icon() }
        };

        const QList<QByteArray> dynamicProps = component->dynamicPropertyNames();
        for (const QByteArray &propName : dynamicProps) {
            const QString key = QString::fromUtf8(propName);
            if (!key.isEmpty())
                entry.insert(key, component->property(propName.constData()));
        }

        components.append(entry);
    }

    QVariantList connections;
    const QList<ConnectionModel *> connectionList = graph ? graph->connectionList() : QList<ConnectionModel *>();
    connections.reserve(connectionList.size());
    for (ConnectionModel *connection : connectionList) {
        if (!connection)
            continue;

        connections.append(QVariantMap{
            { QStringLiteral("id"), connection->id() },
            { QStringLiteral("sourceId"), connection->sourceId() },
            { QStringLiteral("targetId"), connection->targetId() },
            { QStringLiteral("label"), connection->label() },
            { QStringLiteral("sourceSide"), static_cast<int>(connection->sourceSide()) },
            { QStringLiteral("targetSide"), static_cast<int>(connection->targetSide()) }
        });
    }

    return QVariantMap{
        { QStringLiteral("components"), components },
        { QStringLiteral("connections"), connections }
    };
}

bool ValidationService::issueIsError(const QVariantMap &issue)
{
    const QString severity = issue.value(QStringLiteral("severity")).toString().trimmed().toLower();
    return severity.isEmpty() || severity == QStringLiteral("error");
}
