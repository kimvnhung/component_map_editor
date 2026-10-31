#include "component_map_runtime/DeploymentValidator.h"

#include <QHash>
#include <QSet>
#include <QString>

namespace
{
using namespace cme::runtime::v1;

void addIssue(DeploymentValidationResult *result,
              const char *code,
              const char *path,
              const QString &message)
{
    auto *issue = result->add_issues();
    issue->set_code(code);
    issue->set_severity(DEPLOYMENT_VALIDATION_SEVERITY_ERROR);
    issue->set_field_path(path);
    issue->set_message(message.toStdString());
}

struct PortInfo
{
    PortDirection direction = PORT_DIRECTION_UNSPECIFIED;
    QString messageType;
};
}

namespace cme::runtime
{

DeploymentValidator::DeploymentValidator(const RuntimeHandlerRegistry *registry)
    : m_registry(registry)
{
}

cme::runtime::v1::DeploymentValidationResult DeploymentValidator::validate(
    const cme::runtime::v1::DeploymentGraph &graph) const
{
    DeploymentValidationResult result;
    QHash<QString, QHash<QString, PortInfo>> portsByInstance;
    QSet<QString> componentIds;
    QSet<QString> connectionIds;

    if (graph.graph_version().empty())
    {
        addIssue(&result, "GRAPH_VERSION_REQUIRED", "graph_version",
                 QStringLiteral("Graph version must not be empty."));
    }
    if (!m_registry)
    {
        addIssue(&result, "HANDLER_REGISTRY_REQUIRED", "handler_registry",
                 QStringLiteral("A handler registry is required to validate component types."));
    }

    for (int i = 0; i < graph.components_size(); ++i)
    {
        const auto &component = graph.components(i);
        const QString instanceId = QString::fromStdString(component.instance_id());
        const QString componentPath = QStringLiteral("components[%1]").arg(i);
        if (instanceId.isEmpty())
        {
            addIssue(&result, "INSTANCE_ID_REQUIRED",
                     (componentPath + QStringLiteral(".instance_id")).toUtf8().constData(),
                     QStringLiteral("Component instance ID must not be empty."));
        }
        else if (componentIds.contains(instanceId))
        {
            addIssue(&result, "DUPLICATE_INSTANCE_ID",
                     (componentPath + QStringLiteral(".instance_id")).toUtf8().constData(),
                     QStringLiteral("Component instance ID is duplicated."));
        }
        componentIds.insert(instanceId);

        if (component.component_type().empty())
        {
            addIssue(&result, "COMPONENT_TYPE_REQUIRED",
                     (componentPath + QStringLiteral(".component_type")).toUtf8().constData(),
                     QStringLiteral("Component type must not be empty."));
        }
        else if (m_registry && !m_registry->contains(QString::fromStdString(component.component_type())))
        {
            addIssue(&result, "HANDLER_NOT_FOUND",
                     (componentPath + QStringLiteral(".component_type")).toUtf8().constData(),
                     QStringLiteral("Component type has no registered handler."));
        }
        if (component.state_schema_version() == 0)
        {
            addIssue(&result, "STATE_SCHEMA_VERSION_REQUIRED",
                     (componentPath + QStringLiteral(".state_schema_version")).toUtf8().constData(),
                     QStringLiteral("State schema version must be greater than zero."));
        }

        auto &ports = portsByInstance[instanceId];
        for (int p = 0; p < component.ports_size(); ++p)
        {
            const auto &port = component.ports(p);
            const QString portId = QString::fromStdString(port.port_id());
            const QString portPath = componentPath + QStringLiteral(".ports[%1]").arg(p);
            if (portId.isEmpty())
            {
                addIssue(&result, "PORT_ID_REQUIRED",
                         (portPath + QStringLiteral(".port_id")).toUtf8().constData(),
                         QStringLiteral("Port ID must not be empty."));
            }
            else if (ports.contains(portId))
            {
                addIssue(&result, "DUPLICATE_PORT_ID", portPath.toUtf8().constData(),
                         QStringLiteral("Port ID is duplicated within the component."));
            }
            ports.insert(portId, {port.direction(), QString::fromStdString(port.message_type())});
            if (port.direction() == PORT_DIRECTION_UNSPECIFIED)
            {
                addIssue(&result, "PORT_DIRECTION_REQUIRED",
                         (portPath + QStringLiteral(".direction")).toUtf8().constData(),
                         QStringLiteral("Port direction must be specified."));
            }
            if (port.message_type().empty())
            {
                addIssue(&result, "MESSAGE_TYPE_REQUIRED",
                         (portPath + QStringLiteral(".message_type")).toUtf8().constData(),
                         QStringLiteral("Port message type must not be empty."));
            }
        }
    }

    for (int i = 0; i < graph.routes_size(); ++i)
    {
        const auto &route = graph.routes(i);
        const QString routePath = QStringLiteral("routes[%1]").arg(i);
        const QString connectionId = QString::fromStdString(route.connection_id());
        const QString sourceId = QString::fromStdString(route.source_instance_id());
        const QString targetId = QString::fromStdString(route.target_instance_id());
        const QString sourcePort = QString::fromStdString(route.source_port_id());
        const QString targetPort = QString::fromStdString(route.target_port_id());
        const QString messageType = QString::fromStdString(route.message_type());

        if (connectionId.isEmpty())
        {
            addIssue(&result, "CONNECTION_ID_REQUIRED",
                     (routePath + QStringLiteral(".connection_id")).toUtf8().constData(),
                     QStringLiteral("Connection ID must not be empty."));
        }
        else if (connectionIds.contains(connectionId))
        {
            addIssue(&result, "DUPLICATE_CONNECTION_ID",
                     (routePath + QStringLiteral(".connection_id")).toUtf8().constData(),
                     QStringLiteral("Connection ID is duplicated."));
        }
        connectionIds.insert(connectionId);

        if (!componentIds.contains(sourceId))
        {
            addIssue(&result, "SOURCE_INSTANCE_NOT_FOUND",
                     (routePath + QStringLiteral(".source_instance_id")).toUtf8().constData(),
                     QStringLiteral("Route source instance does not exist."));
        }
        if (!componentIds.contains(targetId))
        {
            addIssue(&result, "TARGET_INSTANCE_NOT_FOUND",
                     (routePath + QStringLiteral(".target_instance_id")).toUtf8().constData(),
                     QStringLiteral("Route target instance does not exist."));
        }

        const auto sourcePorts = portsByInstance.value(sourceId);
        const auto targetPorts = portsByInstance.value(targetId);
        if (!sourcePorts.contains(sourcePort))
        {
            addIssue(&result, "SOURCE_PORT_NOT_FOUND",
                     (routePath + QStringLiteral(".source_port_id")).toUtf8().constData(),
                     QStringLiteral("Route source port does not exist."));
        }
        else if (sourcePorts.value(sourcePort).direction != PORT_DIRECTION_OUTPUT)
        {
            addIssue(&result, "SOURCE_PORT_DIRECTION_INVALID",
                     (routePath + QStringLiteral(".source_port_id")).toUtf8().constData(),
                     QStringLiteral("Route source port must be an output port."));
        }
        if (!targetPorts.contains(targetPort))
        {
            addIssue(&result, "TARGET_PORT_NOT_FOUND",
                     (routePath + QStringLiteral(".target_port_id")).toUtf8().constData(),
                     QStringLiteral("Route target port does not exist."));
        }
        else if (targetPorts.value(targetPort).direction != PORT_DIRECTION_INPUT)
        {
            addIssue(&result, "TARGET_PORT_DIRECTION_INVALID",
                     (routePath + QStringLiteral(".target_port_id")).toUtf8().constData(),
                     QStringLiteral("Route target port must be an input port."));
        }
        if (messageType.isEmpty())
        {
            addIssue(&result, "ROUTE_MESSAGE_TYPE_REQUIRED",
                     (routePath + QStringLiteral(".message_type")).toUtf8().constData(),
                     QStringLiteral("Route message type must not be empty."));
        }
        if (sourcePorts.contains(sourcePort) && targetPorts.contains(targetPort)
            && sourcePorts.value(sourcePort).messageType != messageType)
        {
            addIssue(&result, "SOURCE_MESSAGE_TYPE_MISMATCH",
                     (routePath + QStringLiteral(".message_type")).toUtf8().constData(),
                     QStringLiteral("Route message type does not match source port."));
        }
        if (sourcePorts.contains(sourcePort) && targetPorts.contains(targetPort)
            && targetPorts.value(targetPort).messageType != messageType)
        {
            addIssue(&result, "TARGET_MESSAGE_TYPE_MISMATCH",
                     (routePath + QStringLiteral(".message_type")).toUtf8().constData(),
                     QStringLiteral("Route message type does not match target port."));
        }
    }

    result.set_valid(result.issues_size() == 0);
    return result;
}

} // namespace cme::runtime
