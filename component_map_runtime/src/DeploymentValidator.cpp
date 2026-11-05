#include "component_map_runtime/DeploymentValidator.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <functional>

using cme::runtime::v1::GraphDefinition;

namespace
{
using namespace cme::runtime::v1;

void addIssue(DeploymentValidationResult *result,
              const char *code,
              const char *path,
              const QString &message,
              DeploymentValidationSeverity severity = DEPLOYMENT_VALIDATION_SEVERITY_ERROR,
              const QString &identity = {},
              const QString &version = {})
{
    auto *issue = result->add_issues();
    issue->set_code(code);
    issue->set_severity(severity);
    issue->set_field_path(path);
    issue->set_message(message.toStdString());
    if (!identity.isEmpty())
        issue->set_component_identity(identity.toStdString());
    if (!version.isEmpty())
        issue->set_component_version(version.toStdString());
}

struct PortInfo
{
    PortDirection direction = PORT_DIRECTION_UNSPECIFIED;
    QString messageType;
};
}

namespace cme::runtime
{

DeploymentValidator::DeploymentValidator(const RuntimeHandlerRegistry *registry,
                                             const ComponentCatalog *catalog)
    : m_registry(registry), m_catalog(catalog)
{
}

cme::runtime::v1::DeploymentValidationResult DeploymentValidator::validate(
    const cme::runtime::v1::DeploymentGraph &graph) const
{
    DeploymentValidationResult result;
    QHash<QString, QHash<QString, PortInfo>> portsByInstance;
    QSet<QString> componentIds;
    QSet<QString> connectionIds;
    QHash<QString, const GraphDefinition *> definitions;
    QHash<QString, QStringList> definitionDependencies;

    for (int i = 0; i < graph.graph_definitions_size(); ++i)
    {
        const auto &definition = graph.graph_definitions(i);
        const QString id = QString::fromStdString(definition.graph_id());
        const QString version = QString::fromStdString(definition.graph_version());
        const QString key = id + QStringLiteral("@") + version;
        if (id.isEmpty() || version.isEmpty())
        {
            addIssue(&result, "GRAPH_DEFINITION_VERSION_REQUIRED",
                     QStringLiteral("graph_definitions[%1]").arg(i).toUtf8().constData(),
                     QStringLiteral("Graph definition id and version are required."));
            continue;
        }
        if (definitions.contains(key))
            addIssue(&result, "DUPLICATE_GRAPH_DEFINITION", "graph_definitions",
                     QStringLiteral("Graph definition is duplicated."));
        definitions.insert(key, &definition);
        for (const auto &child : definition.components())
        {
            if (!child.composite_graph_id().empty() && !child.composite_graph_version().empty())
                definitionDependencies[key].append(QString::fromStdString(child.composite_graph_id())
                                                    + QStringLiteral("@")
                                                    + QString::fromStdString(child.composite_graph_version()));
        }
    }

    QSet<QString> visiting;
    QSet<QString> visited;
    std::function<void(const QString &)> visit = [&](const QString &key) {
        if (visited.contains(key)) return;
        if (visiting.contains(key))
        {
            addIssue(&result, "GRAPH_DEPENDENCY_CYCLE", "graph_definitions",
                     QStringLiteral("Graph definition dependency cycle detected."));
            return;
        }
        visiting.insert(key);
        for (const auto &dependency : definitionDependencies.value(key))
        {
            if (!definitions.contains(dependency))
                addIssue(&result, "GRAPH_DEFINITION_NOT_FOUND", "graph_definitions",
                         QStringLiteral("Referenced graph definition was not supplied: %1").arg(dependency));
            else
                visit(dependency);
        }
        visiting.remove(key);
        visited.insert(key);
    };
    for (auto it = definitions.cbegin(); it != definitions.cend(); ++it)
        visit(it.key());

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

        const bool isComposite = !component.composite_graph_id().empty()
                                 || !component.composite_graph_version().empty();
        if (isComposite)
        {
            if (component.composite_graph_id().empty() || component.composite_graph_version().empty())
            {
                addIssue(&result, "GRAPH_REFERENCE_INCOMPLETE",
                         (componentPath + QStringLiteral(".composite_graph_id")).toUtf8().constData(),
                         QStringLiteral("Composite graph id and version must be provided together."));
            }
            else
            {
                const QString key = QString::fromStdString(component.composite_graph_id())
                                    + QStringLiteral("@")
                                    + QString::fromStdString(component.composite_graph_version());
                if (!definitions.contains(key))
                    addIssue(&result, "GRAPH_DEFINITION_NOT_FOUND",
                             (componentPath + QStringLiteral(".composite_graph_version")).toUtf8().constData(),
                             QStringLiteral("Referenced composite graph definition was not supplied."));
            }
        }
        else if (component.component_type().empty())
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

        const QString componentIdentity = QString::fromStdString(component.component_identity());
        const QString componentVersion = QString::fromStdString(component.component_version());
        if (m_catalog && !componentIdentity.isEmpty())
        {
            if (componentVersion.isEmpty())
            {
                addIssue(&result, "COMPONENT_VERSION_REQUIRED",
                         (componentPath + QStringLiteral(".component_version")).toUtf8().constData(),
                         QStringLiteral("Component version is required when component identity is specified."));
            }
            else
            {
                QString resolutionError;
                const ComponentDescriptor resolved =
                    m_catalog->resolve(componentIdentity, componentVersion, &resolutionError);
                if (resolved.identity.isEmpty())
                {
                    addIssue(&result, "COMPONENT_DESCRIPTOR_NOT_FOUND",
                             (componentPath + QStringLiteral(".component_version")).toUtf8().constData(),
                             resolutionError);
                }
                else
                {
                    if (resolved.status != ComponentStatus::Available)
                        addIssue(&result, "COMPONENT_UNAVAILABLE",
                                 componentPath.toUtf8().constData(),
                                 QStringLiteral("Component descriptor is not available for execution."),
                                 DEPLOYMENT_VALIDATION_SEVERITY_ERROR,
                                 componentIdentity, componentVersion);
                    if (resolved.source == ComponentSource::Network
                        && resolved.trustState != TrustState::Trusted)
                    {
                        addIssue(&result, "COMPONENT_UNTRUSTED",
                                 componentPath.toUtf8().constData(),
                                 QStringLiteral("Network component descriptor is not trusted."),
                                 DEPLOYMENT_VALIDATION_SEVERITY_ERROR,
                                 componentIdentity, componentVersion);
                    }
                    for (const auto &schema : resolved.properties)
                    {
                        const QString propertyPath = componentPath + QStringLiteral(".properties.") + schema.id;
                        const auto propertyIt = component.properties().find(schema.id.toStdString());
                        const bool hasValue = propertyIt != component.properties().end();
                        if (schema.required && !hasValue && schema.defaultValue.isEmpty())
                            addIssue(&result, "REQUIRED_PROPERTY_MISSING", propertyPath.toUtf8().constData(),
                                     QStringLiteral("Required component property is missing."));
                        if (hasValue)
                        {
                            const QString rawValue = QString::fromStdString(propertyIt->second);
                            const QString type = schema.type.trimmed().toLower();
                            bool typeValid = true;
                            if (type == QStringLiteral("boolean") || type == QStringLiteral("bool"))
                                typeValid = rawValue == QStringLiteral("true") || rawValue == QStringLiteral("false");
                            else if (type == QStringLiteral("number") || type == QStringLiteral("double"))
                                rawValue.toDouble(&typeValid);
                            else if (type == QStringLiteral("integer") || type == QStringLiteral("int"))
                                rawValue.toLongLong(&typeValid);
                            if (!typeValid)
                                addIssue(&result, "PROPERTY_TYPE_INVALID", propertyPath.toUtf8().constData(),
                                         QStringLiteral("Component property value has an invalid type."),
                                         DEPLOYMENT_VALIDATION_SEVERITY_ERROR,
                                         componentIdentity, componentVersion);

                            if (schema.constraints.contains(QStringLiteral("enum"))
                                && !schema.constraints.value(QStringLiteral("enum"))
                                       .split(',', Qt::SkipEmptyParts)
                                       .contains(rawValue))
                            {
                                addIssue(&result, "PROPERTY_ENUM_INVALID", propertyPath.toUtf8().constData(),
                                         QStringLiteral("Component property value is not in the allowed enum."),
                                         DEPLOYMENT_VALIDATION_SEVERITY_ERROR,
                                         componentIdentity, componentVersion);
                            }
                        }
                    }
                    for (const auto &portSchema : resolved.ports)
                    {
                        bool present = false;
                        for (const auto &port : component.ports())
                        {
                            if (QString::fromStdString(port.port_id()) == portSchema.id)
                            {
                                present = true;
                                const QString actualType = port.message_type().empty()
                                    ? QString::fromStdString(port.token_key())
                                    : QString::fromStdString(port.message_type());
                                if (actualType != portSchema.payloadType)
                                    addIssue(&result, "COMPONENT_PORT_TYPE_MISMATCH",
                                             (componentPath + QStringLiteral(".ports.") + portSchema.id).toUtf8().constData(),
                                             QStringLiteral("Component port payload type does not match descriptor."),
                                             DEPLOYMENT_VALIDATION_SEVERITY_ERROR,
                                             componentIdentity, componentVersion);
                                break;
                            }
                        }
                        if (portSchema.required && !present)
                            addIssue(&result, "REQUIRED_PORT_MISSING", componentPath.toUtf8().constData(),
                                     QStringLiteral("Required component port is missing."));
                    }
                }
            }
        }
        if (isComposite)
        {
            const QString key = QString::fromStdString(component.composite_graph_id())
                                + QStringLiteral("@")
                                + QString::fromStdString(component.composite_graph_version());
            const auto *definition = definitions.value(key, nullptr);
            if (definition)
            {
                QSet<QString> exposedPorts;
                for (const auto &port : definition->exposed_ports())
                {
                    const QString portId = QString::fromStdString(port.port_id());
                    if (exposedPorts.contains(portId))
                        addIssue(&result, "DUPLICATE_EXPOSED_PORT", componentPath.toUtf8().constData(),
                                 QStringLiteral("Composite exposed port is duplicated."));
                    exposedPorts.insert(portId);
                }
            }
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
            const QString portMessageType = port.message_type().empty()
                                                ? QString::fromStdString(port.token_key())
                                                : QString::fromStdString(port.message_type());
            ports.insert(portId, {port.direction(), portMessageType});
            if (port.direction() == PORT_DIRECTION_UNSPECIFIED)
            {
                addIssue(&result, "PORT_DIRECTION_REQUIRED",
                         (portPath + QStringLiteral(".direction")).toUtf8().constData(),
                         QStringLiteral("Port direction must be specified."));
            }
            if (port.message_type().empty() && port.token_key().empty())
            {
                addIssue(&result, "MESSAGE_TYPE_REQUIRED",
                         (portPath + QStringLiteral(".message_type")).toUtf8().constData(),
                         QStringLiteral("Port message type must not be empty."));
            }
            if (port.cardinality() == PORT_CARDINALITY_UNSPECIFIED && !graph.graph_id().empty())
            {
                addIssue(&result, "PORT_CARDINALITY_DEFAULTED",
                         (portPath + QStringLiteral(".cardinality")).toUtf8().constData(),
                         QStringLiteral("Port cardinality is unspecified and defaults to single."),
                         DEPLOYMENT_VALIDATION_SEVERITY_WARNING);
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
        const QString routeMessageType = messageType.isEmpty()
                                             ? QString::fromStdString(route.token_key())
                                             : messageType;
        const bool legacyRoute = messageType.isEmpty() && !route.token_key().empty();
        if (legacyRoute && !graph.graph_id().empty())
        {
            addIssue(&result, "TOKEN_KEY_DEPRECATED", routePath.toUtf8().constData(),
                     QStringLiteral("tokenKey is deprecated; use message_type."),
                     DEPLOYMENT_VALIDATION_SEVERITY_WARNING);
        } else if (messageType.isEmpty() && route.token_key().empty())
        {
            addIssue(&result, "ROUTE_MESSAGE_TYPE_REQUIRED",
                     (routePath + QStringLiteral(".message_type")).toUtf8().constData(),
                     QStringLiteral("Route message type must not be empty."));
        }
        if (!route.enabled())
        {
            addIssue(&result, "ROUTE_DISABLED", routePath.toUtf8().constData(),
                     QStringLiteral("Route is disabled."),
                     DEPLOYMENT_VALIDATION_SEVERITY_WARNING);
        }
        if (route.delivery_mode() == DELIVERY_MODE_UNSPECIFIED && !graph.graph_id().empty())
        {
            addIssue(&result, "DELIVERY_MODE_DEFAULTED",
                     (routePath + QStringLiteral(".delivery_mode")).toUtf8().constData(),
                     QStringLiteral("Delivery mode is unspecified and defaults to at-most-once."),
                     DEPLOYMENT_VALIDATION_SEVERITY_WARNING);
        }
        if (!route.ordering_key().empty() && routeMessageType.isEmpty())
        {
            addIssue(&result, "ORDERING_KEY_WITHOUT_MESSAGE_TYPE", routePath.toUtf8().constData(),
                     QStringLiteral("Ordering key requires a message type."));
        }

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
        if (routeMessageType.isEmpty())
        {
            addIssue(&result, "ROUTE_MESSAGE_TYPE_REQUIRED",
                     (routePath + QStringLiteral(".message_type")).toUtf8().constData(),
                     QStringLiteral("Route message type must not be empty."));
        }
        if (sourcePorts.contains(sourcePort) && targetPorts.contains(targetPort)
            && sourcePorts.value(sourcePort).messageType != routeMessageType)
        {
            addIssue(&result, "SOURCE_MESSAGE_TYPE_MISMATCH",
                     (routePath + QStringLiteral(".message_type")).toUtf8().constData(),
                     QStringLiteral("Route message type does not match source port."));
        }
        if (sourcePorts.contains(sourcePort) && targetPorts.contains(targetPort)
            && targetPorts.value(targetPort).messageType != routeMessageType)
        {
            addIssue(&result, "TARGET_MESSAGE_TYPE_MISMATCH",
                     (routePath + QStringLiteral(".message_type")).toUtf8().constData(),
                     QStringLiteral("Route message type does not match target port."));
        }
    }

    bool hasError = false;
    for (const auto &issue : result.issues())
    {
        if (issue.severity() == DEPLOYMENT_VALIDATION_SEVERITY_ERROR)
        {
            hasError = true;
            break;
        }
    }
    result.set_valid(!hasError);
    return result;
}

} // namespace cme::runtime
