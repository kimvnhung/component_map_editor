#include "component_map_runtime/FlattenCompiler.h"

#include <QCryptographicHash>
#include <QIODevice>
#include <QSet>
#include <QTextStream>
#include <algorithm>
#include <functional>

using namespace cme::runtime;
using namespace cme::runtime::v1;

namespace
{
void issue(FlattenResult &result, const char *code, const QString &path, const QString &message)
{
    result.issues.append({QString::fromLatin1(code), path, message});
}

QString definitionKey(const QString &id, const QString &version)
{
    return id + QStringLiteral("@") + version;
}

QString definitionKey(const GraphDefinition &definition)
{
    return definitionKey(QString::fromStdString(definition.graph_id()),
                         QString::fromStdString(definition.graph_version()));
}

struct Endpoint
{
    QString path;
    QString port;
    QString definition;
};

struct PortDescriptor
{
    PortDirection direction = PORT_DIRECTION_UNSPECIFIED;
    QString messageType;
};

QMap<QString, PortDescriptor> componentPorts(const ComponentInstance &component)
{
    QMap<QString, PortDescriptor> result;
    for (const auto &port : component.ports())
    {
        const QString messageType = port.message_type().empty()
                                         ? QString::fromStdString(port.token_key())
                                         : QString::fromStdString(port.message_type());
        result.insert(QString::fromStdString(port.port_id()), {port.direction(), messageType});
    }
    return result;
}
}

FlattenCompiler::FlattenCompiler(FlattenLimits limits)
    : m_limits(limits)
{
}

FlattenResult FlattenCompiler::compile(const DeploymentGraph &deployment) const
{
    FlattenResult result;
    if (deployment.ByteSizeLong() > m_limits.maxSerializedBytes)
    {
        issue(result, "GRAPH_DEPLOYMENT_SIZE_EXCEEDED", {},
              QStringLiteral("Deployment size exceeds limit."));
        return result;
    }

    QMap<QString, const GraphDefinition *> definitions;
    for (const auto &definition : deployment.graph_definitions())
    {
        const QString key = definitionKey(definition);
        if (key == QStringLiteral("@") || definition.graph_id().empty()
            || definition.graph_version().empty())
        {
            issue(result, "GRAPH_DEFINITION_VERSION_REQUIRED", key,
                  QStringLiteral("Graph definition id and version are required."));
            continue;
        }
        if (definitions.contains(key))
        {
            issue(result, "DUPLICATE_GRAPH_DEFINITION", key,
                  QStringLiteral("Duplicate graph definition."));
            continue;
        }
        definitions.insert(key, &definition);
    }

    QSet<QString> visiting;
    QSet<QString> visited;
    std::function<void(const QString &)> checkDependencies = [&](const QString &key) {
        if (visited.contains(key))
            return;
        if (visiting.contains(key))
        {
            issue(result, "GRAPH_DEPENDENCY_CYCLE", key,
                  QStringLiteral("Graph definition dependency cycle."));
            return;
        }
        const auto definition = definitions.value(key, nullptr);
        if (!definition)
            return;
        visiting.insert(key);
        for (const auto &component : definition->components())
        {
            const bool hasId = !component.composite_graph_id().empty();
            const bool hasVersion = !component.composite_graph_version().empty();
            if (hasId != hasVersion)
            {
                issue(result, "GRAPH_REFERENCE_INCOMPLETE", key,
                      QStringLiteral("Composite graph id and version must be provided together."));
                continue;
            }
            if (hasId)
            {
                const QString childKey = definitionKey(
                    QString::fromStdString(component.composite_graph_id()),
                    QString::fromStdString(component.composite_graph_version()));
                if (!definitions.contains(childKey))
                    issue(result, "GRAPH_DEFINITION_NOT_FOUND", key,
                          QStringLiteral("Referenced graph definition was not supplied: %1")
                              .arg(childKey));
                else
                    checkDependencies(childKey);
            }
        }
        visiting.remove(key);
        visited.insert(key);
    };

    for (auto it = definitions.cbegin(); it != definitions.cend(); ++it)
        checkDependencies(it.key());

    auto validateDefinition = [&](const GraphDefinition &definition) {
        const QString definitionPath = definitionKey(definition);
        QMap<QString, const ComponentInstance *> components;
        for (const auto &component : definition.components())
        {
            const QString id = QString::fromStdString(component.instance_id());
            if (id.isEmpty())
            {
                issue(result, "INSTANCE_ID_REQUIRED", definitionPath,
                      QStringLiteral("Component instance ID must not be empty."));
                continue;
            }
            if (components.contains(id))
                issue(result, "DUPLICATE_INSTANCE_ID", definitionPath + QStringLiteral("/") + id,
                      QStringLiteral("Component instance ID is duplicated."));
            else
                components.insert(id, &component);
        }

        QSet<QString> exposed;
        for (const auto &port : definition.exposed_ports())
        {
            const QString portId = QString::fromStdString(port.port_id());
            const QString path = definitionPath + QStringLiteral("/exposed/") + portId;
            if (portId.isEmpty() || exposed.contains(portId))
            {
                issue(result, "EXPOSED_PORT_MAPPING_NOT_ONE_TO_ONE", path,
                      QStringLiteral("Each exposed port must have one unique mapping."));
                continue;
            }
            exposed.insert(portId);
            const auto component = components.value(
                QString::fromStdString(port.internal_instance_id()), nullptr);
            if (!component)
            {
                issue(result, "EXPOSED_PORT_INTERNAL_ENDPOINT_NOT_FOUND", path,
                      QStringLiteral("Exposed port internal instance does not exist."));
                continue;
            }
            const auto ports = componentPorts(*component);
            const QString internalPort = QString::fromStdString(port.internal_port_id());
            if (!ports.contains(internalPort))
            {
                issue(result, "EXPOSED_PORT_INTERNAL_ENDPOINT_NOT_FOUND", path,
                      QStringLiteral("Exposed port internal port does not exist."));
                continue;
            }
            const auto descriptor = ports.value(internalPort);
            const QString exposedType = port.message_type().empty()
                                             ? QString()
                                             : QString::fromStdString(port.message_type());
            if (descriptor.direction != port.direction()
                || (!exposedType.isEmpty() && descriptor.messageType != exposedType))
            {
                issue(result, "EXPOSED_PORT_MAPPING_MISMATCH", path,
                      QStringLiteral("Exposed port direction and message type must match its internal port."));
            }
        }
    };

    for (const auto &definition : deployment.graph_definitions())
        validateDefinition(definition);

    if (!result.issues.isEmpty())
        return result;

    GraphDefinition root;
    root.set_graph_id(deployment.graph_id());
    root.set_graph_version(deployment.graph_version());
    for (const auto &component : deployment.components())
        *root.add_components() = component;
    for (const auto &route : deployment.routes())
        *root.add_routes() = route;

    std::function<Endpoint(const GraphDefinition &, const QString &, const QString &, const QString &, int)> resolve;
    resolve = [&](const GraphDefinition &graph, const QString &prefix, const QString &instanceId,
                  const QString &portId, int depth) -> Endpoint {
        if (depth > m_limits.maxDepth)
        {
            issue(result, "GRAPH_DEPTH_EXCEEDED", prefix,
                  QStringLiteral("Maximum graph depth exceeded."));
            return {};
        }
        const ComponentInstance *component = nullptr;
        for (const auto &candidate : graph.components())
        {
            if (candidate.instance_id() == instanceId.toStdString())
            {
                component = &candidate;
                break;
            }
        }
        if (!component)
        {
            issue(result, "BOUNDARY_ENDPOINT_NOT_FOUND", prefix,
                  QStringLiteral("Endpoint instance not found."));
            return {};
        }
        if (component->composite_graph_id().empty())
        {
            const QString graphKey = definitionKey(QString::fromStdString(graph.graph_id()),
                                                    QString::fromStdString(graph.graph_version()));
            return {prefix + QStringLiteral("/") + instanceId, portId, graphKey};
        }

        const QString childKey = definitionKey(
            QString::fromStdString(component->composite_graph_id()),
            QString::fromStdString(component->composite_graph_version()));
        const auto child = definitions.value(childKey, nullptr);
        if (!child)
        {
            issue(result, "GRAPH_DEFINITION_NOT_FOUND", prefix + QStringLiteral("/") + instanceId,
                  QStringLiteral("Missing graph definition."));
            return {};
        }
        const ExposedPort *exposed = nullptr;
        for (const auto &candidate : child->exposed_ports())
        {
            if (candidate.port_id() == portId.toStdString())
            {
                exposed = &candidate;
                break;
            }
        }
        if (!exposed)
        {
            issue(result, "BOUNDARY_PORT_NOT_EXPOSED",
                  prefix + QStringLiteral("/") + instanceId + QStringLiteral("/") + portId,
                  QStringLiteral("Port is not exposed by the composite definition."));
            return {};
        }
        return resolve(*child, prefix + QStringLiteral("/") + instanceId,
                       QString::fromStdString(exposed->internal_instance_id()),
                       QString::fromStdString(exposed->internal_port_id()), depth + 1);
    };

    std::function<void(const GraphDefinition &, const QString &, int)> expand;
    expand = [&](const GraphDefinition &graph, const QString &prefix, int depth) {
        if (depth > m_limits.maxDepth)
        {
            issue(result, "GRAPH_DEPTH_EXCEEDED", prefix,
                  QStringLiteral("Maximum graph depth exceeded."));
            return;
        }
        const QString scope = definitionKey(QString::fromStdString(graph.graph_id()),
                                            QString::fromStdString(graph.graph_version()));
        for (const auto &component : graph.components())
        {
            const QString id = QString::fromStdString(component.instance_id());
            if (component.composite_graph_id().empty())
            {
                FlattenLeaf leaf;
                leaf.qualifiedPath = prefix + QStringLiteral("/") + id;
                leaf.componentType = QString::fromStdString(component.component_type());
                leaf.graphScope = scope;
                leaf.definitionId = QString::fromStdString(graph.graph_id());
                leaf.definitionVersion = QString::fromStdString(graph.graph_version());
                leaf.stateSchemaVersion = component.state_schema_version();
                for (auto it = component.properties().cbegin(); it != component.properties().cend(); ++it)
                    leaf.resolvedProperties.insert(QString::fromStdString(it->first),
                                                   QString::fromStdString(it->second));
                result.plan.leaves.append(leaf);
            }
            else
            {
                const QString childKey = definitionKey(
                    QString::fromStdString(component.composite_graph_id()),
                    QString::fromStdString(component.composite_graph_version()));
                const auto child = definitions.value(childKey, nullptr);
                if (!child)
                {
                    issue(result, "GRAPH_DEFINITION_NOT_FOUND", prefix + QStringLiteral("/") + id,
                          QStringLiteral("Missing graph definition."));
                }
                else
                    expand(*child, prefix + QStringLiteral("/") + id, depth + 1);
            }
        }
        for (const auto &route : graph.routes())
        {
            const Endpoint source = resolve(graph, prefix,
                                            QString::fromStdString(route.source_instance_id()),
                                            QString::fromStdString(route.source_port_id()), depth);
            const Endpoint target = resolve(graph, prefix,
                                            QString::fromStdString(route.target_instance_id()),
                                            QString::fromStdString(route.target_port_id()), depth);
            if (source.path.isEmpty() || target.path.isEmpty())
                continue;
            FlattenRoute flattened;
            flattened.routeId = QString::fromStdString(route.connection_id());
            flattened.sourceLeafPath = source.path;
            flattened.sourcePort = source.port;
            flattened.targetLeafPath = target.path;
            flattened.targetPort = target.port;
            flattened.messageType = route.message_type().empty()
                                        ? QString::fromStdString(route.token_key())
                                        : QString::fromStdString(route.message_type());
            flattened.sourceDefinitionPath = source.definition;
            flattened.targetDefinitionPath = target.definition;
            flattened.boundaryPath = prefix;
            result.plan.routes.append(flattened);
        }
    };

    expand(root, QStringLiteral("root"), 0);
    if (result.plan.leaves.size() > m_limits.maxLeaves
        || result.plan.routes.size() > m_limits.maxRoutes)
    {
        issue(result, "GRAPH_EXPANSION_LIMIT_EXCEEDED", {},
              QStringLiteral("Expansion limit exceeded."));
    }

    std::sort(result.plan.leaves.begin(), result.plan.leaves.end(),
              [](const auto &left, const auto &right) { return left.qualifiedPath < right.qualifiedPath; });
    std::sort(result.plan.routes.begin(), result.plan.routes.end(),
              [](const auto &left, const auto &right) { return left.routeId < right.routeId; });

    QByteArray canonical;
    QTextStream stream(&canonical, QIODevice::WriteOnly);
    stream << "graph\n" << deployment.graph_id().c_str() << "\n"
           << deployment.graph_version().c_str() << "\n";
    for (const auto &leaf : result.plan.leaves)
    {
        stream << "leaf\n" << leaf.qualifiedPath << '\n' << leaf.componentType << '\n'
               << leaf.graphScope << '\n' << leaf.stateSchemaVersion << '\n';
        for (auto it = leaf.resolvedProperties.cbegin(); it != leaf.resolvedProperties.cend(); ++it)
            stream << "property\n" << it.key() << '\n' << it.value() << '\n';
    }
    for (const auto &route : result.plan.routes)
        stream << "route\n" << route.routeId << '\n' << route.sourceLeafPath << '\n'
               << route.sourcePort << '\n' << route.targetLeafPath << '\n' << route.targetPort << '\n'
               << route.messageType << '\n' << route.sourceDefinitionPath << '\n'
               << route.targetDefinitionPath << '\n' << route.boundaryPath << '\n';
    stream.flush();
    result.plan.hash = QCryptographicHash::hash(canonical, QCryptographicHash::Sha256);
    result.valid = result.issues.isEmpty();
    return result;
}
