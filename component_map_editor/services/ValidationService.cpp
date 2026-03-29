#include "ValidationService.h"

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "adapters/ValidationAdapter.h"

#include <QDebug>

ValidationService::ValidationService(QObject *parent)
    : QObject(parent)
{}

void ValidationService::setValidationProviders(const QList<const IValidationProvider *> &providers)
{
    static bool warnedOnce = false;
    if (!warnedOnce) {
        warnedOnce = true;
        qWarning().noquote()
            << "[Phase10] ValidationService::setValidationProviders(V1) is deprecated;"
            << "switch to setValidationProvidersV2() for typed-first path.";
    }

    m_validationV1Adapters.clear();
    m_validationProviders.clear();
    for (const IValidationProvider *provider : providers) {
        if (provider)
            m_validationV1Adapters.emplace_back(new ValidationProviderV1ToV2Adapter(provider));
    }

    for (const std::unique_ptr<ValidationProviderV1ToV2Adapter> &adapter : m_validationV1Adapters) {
        if (adapter)
            m_validationProviders.append(adapter.get());
    }
}

void ValidationService::setValidationProvidersV2(const QList<const IValidationProviderV2 *> &providers)
{
    m_validationV1Adapters.clear();
    m_validationProviders.clear();
    for (const IValidationProviderV2 *provider : providers) {
        if (provider)
            m_validationProviders.append(provider);
    }
}

void ValidationService::rebuildValidationFromRegistry(const ExtensionContractRegistry &registry)
{
    setValidationProvidersV2(registry.validationProvidersV2());
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
            { QStringLiteral("severity"), cme::adapter::validationSeverityToString(cme::VALIDATION_SEVERITY_ERROR) },
            { QStringLiteral("message"), QStringLiteral("Graph is null") },
            { QStringLiteral("entityType"), QStringLiteral("graph") },
            { QStringLiteral("entityId"), QString() }
        });
        return issues;
    }

    if (m_validationProviders.isEmpty())
        return issues;

    // Phase 5: Build typed snapshot internally
    const cme::GraphSnapshot typedSnapshot = buildTypedGraphSnapshot(graph);

    for (const IValidationProviderV2 *provider : std::as_const(m_validationProviders)) {
        if (!provider)
            continue;

        cme::GraphValidationResult providerResult;
        QString providerError;
        if (!provider->validateGraph(typedSnapshot, &providerResult, &providerError)) {
            issues.append(QVariantMap{
                { QStringLiteral("code"), QStringLiteral("CORE_VALIDATION_PROVIDER_ERROR") },
                { QStringLiteral("severity"), cme::adapter::validationSeverityToString(cme::VALIDATION_SEVERITY_ERROR) },
                { QStringLiteral("message"), providerError.isEmpty() ? QStringLiteral("Validation provider failed")
                                                                      : providerError },
                { QStringLiteral("entityType"), QStringLiteral("graph") },
                { QStringLiteral("entityId"), QString() }
            });
            continue;
        }

        for (int i = 0; i < providerResult.issues_size(); ++i) {
            const QVariantMap issue = cme::adapter::validationIssueToVariantMap(providerResult.issues(i));
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

// Phase 5: Build typed GraphSnapshot proto from graph model
cme::GraphSnapshot ValidationService::buildTypedGraphSnapshot(GraphModel *graph) const
{
    cme::GraphSnapshot snapshot;

    if (!graph)
        return snapshot;

    // Convert components
    const QList<ComponentModel *> componentList = graph->componentList();
    for (ComponentModel *component : componentList) {
        if (!component)
            continue;

        auto *componentData = snapshot.add_components();
        componentData->set_id(component->id().toStdString());
        componentData->set_type_id(component->type().toStdString());
        componentData->set_x(component->x());
        componentData->set_y(component->y());

        // Add static properties
        auto *props = componentData->mutable_properties();
        props->insert({"title", component->title().toStdString()});
        props->insert({"width", QString::number(component->width()).toStdString()});
        props->insert({"height", QString::number(component->height()).toStdString()});
        props->insert({"color", component->color().toStdString()});
        props->insert({"shape", component->shape().toStdString()});
        props->insert({"content", component->content().toStdString()});
        props->insert({"icon", component->icon().toStdString()});

        // Add dynamic properties
        const QList<QByteArray> dynamicProps = component->dynamicPropertyNames();
        for (const QByteArray &propName : dynamicProps) {
            const QString key = QString::fromUtf8(propName);
            if (!key.isEmpty()) {
                const QVariant value = component->property(propName.constData());
                props->insert({key.toStdString(), value.toString().toStdString()});
            }
        }
    }

    // Convert connections
    const QList<ConnectionModel *> connectionList = graph->connectionList();
    for (ConnectionModel *connection : connectionList) {
        if (!connection)
            continue;

        auto *connectionData = snapshot.add_connections();
        connectionData->set_id(connection->id().toStdString());
        connectionData->set_source_id(connection->sourceId().toStdString());
        connectionData->set_target_id(connection->targetId().toStdString());

        // Add properties
        auto *props = connectionData->mutable_properties();
        props->insert({"label", connection->label().toStdString()});
        props->insert({"sourceSide", QString::number(static_cast<int>(connection->sourceSide())).toStdString()});
        props->insert({"targetSide", QString::number(static_cast<int>(connection->targetSide())).toStdString()});
    }

    return snapshot;
}
bool ValidationService::issueIsError(const QVariantMap &issue)
{
    const QString severity = issue.value(QStringLiteral("severity")).toString().trimmed().toLower();
    // Compare against the adapter-defined constant; no raw "error" string in core logic.
    return severity.isEmpty()
           || severity == cme::adapter::validationSeverityToString(cme::VALIDATION_SEVERITY_ERROR);
}
