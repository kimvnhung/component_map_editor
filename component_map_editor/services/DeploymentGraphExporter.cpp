#include "DeploymentGraphExporter.h"

#include <QHash>
#include <QSet>

namespace
{

using cme::runtime::v1::DeploymentGraph;

QString stringProperty(const QObject *object, const QString &name)
{
    if (!object)
        return {};

    const QByteArray key = name.toUtf8();
    const QVariant value = object->property(key.constData());
    return value.isValid() ? value.toString().trimmed() : QString();
}

quint32 positiveIntegerProperty(const QObject *object,
                               const QString &name,
                               quint32 fallback)
{
    if (!object)
        return fallback;

    const QByteArray key = name.toUtf8();
    const QVariant value = object->property(key.constData());
    bool ok = false;
    const uint parsed = value.toUInt(&ok);
    return ok && parsed > 0 ? parsed : fallback;
}

void setError(QString *error, const QString &message)
{
    if (error)
        *error = message;
}

QString componentPath(int index, const QString &field = {})
{
    const QString path = QStringLiteral("components[%1]").arg(index);
    return field.isEmpty() ? path : path + QStringLiteral(".") + field;
}

QString routePath(int index, const QString &field = {})
{
    const QString path = QStringLiteral("routes[%1]").arg(index);
    return field.isEmpty() ? path : path + QStringLiteral(".") + field;
}

bool requireValue(const QString &value, const QString &path, QString *error)
{
    if (!value.isEmpty())
        return true;

    setError(error, QStringLiteral("Required deployment field is empty: %1").arg(path));
    return false;
}

} // namespace

namespace cme::editor
{

bool DeploymentGraphExporter::exportGraph(const GraphModel *graph,
                                          const Options &options,
                                          cme::runtime::v1::DeploymentGraph *output,
                                          QString *error)
{
    if (!output)
    {
        setError(error, QStringLiteral("DeploymentGraph output must not be null."));
        return false;
    }
    if (!graph)
    {
        setError(error, QStringLiteral("GraphModel must not be null."));
        return false;
    }
    if (!requireValue(graph->id().trimmed(), QStringLiteral("graph_id"), error))
        return false;
    if (!requireValue(options.graphVersion.trimmed(), QStringLiteral("graph_version"), error))
        return false;
    if (options.defaultStateSchemaVersion == 0)
    {
        setError(error, QStringLiteral("defaultStateSchemaVersion must be greater than zero."));
        return false;
    }

    DeploymentGraph candidate;
    candidate.set_graph_id(graph->id().trimmed().toStdString());
    candidate.set_graph_version(options.graphVersion.trimmed().toStdString());

    QSet<QString> componentIds;
    QHash<QString, const ComponentModel *> componentsById;
    const auto &components = graph->componentList();
    for (int i = 0; i < components.size(); ++i)
    {
        const ComponentModel *component = components.at(i);
        if (!component)
        {
            setError(error, QStringLiteral("Null component at %1.").arg(componentPath(i)));
            return false;
        }

        const QString id = component->id().trimmed();
        const QString type = component->type().trimmed();
        if (!requireValue(id, componentPath(i, QStringLiteral("instance_id")), error)
            || !requireValue(type, componentPath(i, QStringLiteral("component_type")), error))
            return false;
        if (componentIds.contains(id))
        {
            setError(error, QStringLiteral("Duplicate component instance ID: %1").arg(id));
            return false;
        }
        componentIds.insert(id);
        componentsById.insert(id, component);

        auto *deploymentComponent = candidate.add_components();
        deploymentComponent->set_instance_id(id.toStdString());
        deploymentComponent->set_component_type(type.toStdString());
        deploymentComponent->set_state_schema_version(
            positiveIntegerProperty(component, QStringLiteral("stateSchemaVersion"),
                                    options.defaultStateSchemaVersion));

        for (const QByteArray &name : component->dynamicPropertyNames())
        {
            const QString key = QString::fromUtf8(name).trimmed();
            if (key.isEmpty())
                continue;
            deploymentComponent->mutable_properties()->insert(
                {key.toStdString(), component->property(name.constData()).toString().toStdString()});
        }
    }

    QSet<QString> routeIds;
    const auto &connections = graph->connectionList();
    for (int i = 0; i < connections.size(); ++i)
    {
        const ConnectionModel *connection = connections.at(i);
        if (!connection)
        {
            setError(error, QStringLiteral("Null connection at %1.").arg(routePath(i)));
            return false;
        }

        const QString id = connection->id().trimmed();
        const QString sourceId = connection->sourceId().trimmed();
        const QString targetId = connection->targetId().trimmed();
        if (!requireValue(id, routePath(i, QStringLiteral("connection_id")), error)
            || !requireValue(sourceId, routePath(i, QStringLiteral("source_instance_id")), error)
            || !requireValue(targetId, routePath(i, QStringLiteral("target_instance_id")), error))
            return false;
        if (routeIds.contains(id))
        {
            setError(error, QStringLiteral("Duplicate connection ID: %1").arg(id));
            return false;
        }
        if (!componentsById.contains(sourceId) || !componentsById.contains(targetId))
        {
            setError(error, QStringLiteral("Route %1 references an unknown component.").arg(id));
            return false;
        }
        routeIds.insert(id);

        QString sourcePort = stringProperty(connection, QStringLiteral("sourcePort"));
        QString targetPort = stringProperty(connection, QStringLiteral("targetPort"));
        QString messageType = stringProperty(connection, QStringLiteral("messageType"));
        if (sourcePort.isEmpty())
            sourcePort = options.defaultSourcePort.trimmed();
        if (targetPort.isEmpty())
            targetPort = options.defaultTargetPort.trimmed();
        if (messageType.isEmpty())
            messageType = options.defaultMessageType.trimmed();
        if (!requireValue(sourcePort, routePath(i, QStringLiteral("source_port_id")), error)
            || !requireValue(targetPort, routePath(i, QStringLiteral("target_port_id")), error)
            || !requireValue(messageType, routePath(i, QStringLiteral("message_type")), error))
            return false;

        auto *route = candidate.add_routes();
        route->set_connection_id(id.toStdString());
        route->set_source_instance_id(sourceId.toStdString());
        route->set_source_port_id(sourcePort.toStdString());
        route->set_target_instance_id(targetId.toStdString());
        route->set_target_port_id(targetPort.toStdString());
        route->set_message_type(messageType.toStdString());
        route->set_enabled(true);
    }

    *output = std::move(candidate);
    return true;
}

} // namespace cme::editor
