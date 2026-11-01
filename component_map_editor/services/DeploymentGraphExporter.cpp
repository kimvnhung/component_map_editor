#include "DeploymentGraphExporter.h"

#include <QHash>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

namespace
{

using cme::runtime::v1::DeploymentGraph;
using cme::runtime::v1::PortDirection;

QString stringProperty(const QObject *object, const QString &name)
{
    if (!object || name.isEmpty())
        return {};

    const QByteArray key = name.toUtf8();
    const QVariant value = object->property(key.constData());
    return value.isValid() ? value.toString().trimmed() : QString();
}

quint32 positiveIntegerProperty(const QObject *object, const QString &name, quint32 fallback)
{
    if (!object || name.isEmpty())
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

QString messageTypeForPort(const QObject *object,
                           const QString &portsProperty,
                           const QString &portId)
{
    const QByteArray key = portsProperty.toUtf8();
    const QVariant value = object->property(key.constData());
    if (!value.isValid())
        return {};

    const QVariantList ports = value.toList();
    for (const QVariant &entry : ports)
    {
        const QVariantMap port = entry.toMap();
        if (port.value(QStringLiteral("id")).toString().trimmed() == portId)
            return port.value(QStringLiteral("messageType")).toString().trimmed();
    }
    return {};
}

QString solePortId(const QObject *object, const QString &portsProperty)
{
    const QByteArray key = portsProperty.toUtf8();
    const QVariant value = object->property(key.constData());
    if (!value.isValid())
        return {};

    const QVariantList ports = value.toList();
    if (ports.size() != 1)
        return {};

    return ports.first().toMap().value(QStringLiteral("id")).toString().trimmed();
}

bool appendPorts(const ComponentModel *component,
                 const QString &propertyName,
                 PortDirection direction,
                 cme::runtime::v1::ComponentInstance *deploymentComponent,
                 QSet<QString> *knownPorts,
                 QString *error)
{
    const QByteArray key = propertyName.toUtf8();
    const QVariant value = component->property(key.constData());
    if (!value.isValid())
        return true;

    const QVariantList ports = value.toList();
    for (const QVariant &entry : ports)
    {
        const QVariantMap port = entry.toMap();
        const QString id = port.value(QStringLiteral("id")).toString().trimmed();
        const QString messageType = port.value(QStringLiteral("messageType")).toString().trimmed();
        if (!requireValue(id, propertyName + QStringLiteral("[].id"), error)
            || !requireValue(messageType, propertyName + QStringLiteral("[].messageType"), error))
            return false;
        if (knownPorts->contains(id))
        {
            setError(error, QStringLiteral("Port %1 is declared more than once on component %2.")
                                .arg(id, component->id().trimmed()));
            return false;
        }

        knownPorts->insert(id);
        auto *deploymentPort = deploymentComponent->add_ports();
        deploymentPort->set_port_id(id.toStdString());
        deploymentPort->set_direction(direction);
        deploymentPort->set_message_type(messageType.toStdString());
    }
    return true;
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
            positiveIntegerProperty(component, options.stateSchemaVersionProperty,
                                    options.defaultStateSchemaVersion));

        const QString compositeId = stringProperty(component, options.compositeGraphIdProperty);
        const QString compositeVersion = stringProperty(component, options.compositeGraphVersionProperty);
        if (compositeId.isEmpty() != compositeVersion.isEmpty())
        {
            setError(error, QStringLiteral("Composite reference on %1 requires both graph id and version.")
                                .arg(componentPath(i)));
            return false;
        }
        deploymentComponent->set_composite_graph_id(compositeId.toStdString());
        deploymentComponent->set_composite_graph_version(compositeVersion.toStdString());

        QSet<QString> knownPorts;
        if (!appendPorts(component, options.inputPortsProperty,
                         cme::runtime::v1::PORT_DIRECTION_INPUT,
                         deploymentComponent, &knownPorts, error)
            || !appendPorts(component, options.outputPortsProperty,
                            cme::runtime::v1::PORT_DIRECTION_OUTPUT,
                            deploymentComponent, &knownPorts, error))
            return false;

        for (const QByteArray &name : component->dynamicPropertyNames())
        {
            const QString key = QString::fromUtf8(name).trimmed();
            if (key.isEmpty())
                continue;
            if (key == options.inputPortsProperty || key == options.outputPortsProperty)
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

        QString sourcePort = stringProperty(connection, options.sourcePortProperty);
        QString targetPort = stringProperty(connection, options.targetPortProperty);
        QString messageType = stringProperty(connection, options.messageTypeProperty);
        if (options.requireExplicitRouteMetadata
            && !requireValue(sourcePort, routePath(i, QStringLiteral("source_port_id")), error))
            return false;
        if (options.requireExplicitRouteMetadata
            && !requireValue(targetPort, routePath(i, QStringLiteral("target_port_id")), error))
            return false;

        if (!options.requireExplicitRouteMetadata)
        {
            if (sourcePort.isEmpty())
                sourcePort = solePortId(componentsById.value(sourceId),
                                        options.outputPortsProperty);
            if (targetPort.isEmpty())
                targetPort = solePortId(componentsById.value(targetId),
                                        options.inputPortsProperty);
        }

        if (messageType.isEmpty())
        {
            const QString fromSource = messageTypeForPort(componentsById.value(sourceId),
                                                          options.outputPortsProperty, sourcePort);
            const QString fromTarget = messageTypeForPort(componentsById.value(targetId),
                                                          options.inputPortsProperty, targetPort);
            messageType = fromSource.isEmpty() ? fromTarget : fromSource;
        }

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
