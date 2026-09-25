#include "GraphExecutionSandboxSim.h"

#include <QThread>
#include <base_log.h>

#include "Actor.h"

GraphExecutionSandboxSim::GraphExecutionSandboxSim()
    : m_scheduler(new ActorScheduler())
{}

bool GraphExecutionSandboxSim::isValid() const
{
    return !m_components.empty();
}

std::vector<Component *> GraphExecutionSandboxSim::getComponents() const
{
    return m_components;
}

std::vector<Connection *> GraphExecutionSandboxSim::getConnections() const
{
    return m_connections;
}

Component *GraphExecutionSandboxSim::getComponentById(const std::string &id) const
{
    for (Component *component : m_components)
    {
        if (component->getId() == id)
        {
            return component;
        }
    }

    return nullptr;
}

Connection *GraphExecutionSandboxSim::getConnectionById(const std::string &id) const
{
    for (Connection *connection : m_connections)
    {
        if (connection->getId() == id)
        {
            return connection;
        }
    }

    return nullptr;
}


std::vector<Connection *> GraphExecutionSandboxSim::getIncomingConnections(const std::string &componentId) const
{
    std::vector<Connection *> incomingConnections;

    for (Connection *connection : m_connections)
    {
        if (connection->getTargetComponentId() == componentId)
        {
            incomingConnections.push_back(connection);
        }
    }

    return incomingConnections;
}

std::vector<Connection *> GraphExecutionSandboxSim::getOutgoingConnections(const std::string &componentId) const
{
    std::vector<Connection *> outgoingConnections;

    for (Connection *connection : m_connections)
    {
        if (connection->getSourceComponentId() == componentId)
        {
            outgoingConnections.push_back(connection);
        }
    }

    return outgoingConnections;
}

void GraphExecutionSandboxSim::addComponent(Component *component)
{
    m_components.push_back(component);
}

void GraphExecutionSandboxSim::removeComponent(const std::string &id)
{
    // Verify id exists
    auto it = std::find_if(m_components.begin(), m_components.end(),
    [&id](Component * component) { return component->getId() == id; });

    if (it != m_components.end())
    {
        m_components.erase(it);
        // Remove associated connections
        m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(),
        [&id](Connection * connection) { return connection->getSourceComponentId() == id || connection->getTargetComponentId() == id; }),
        m_connections.end());

    }
    else
    {
        LOGWF("[GraphExecutionSandboxSim][ERROR] Component with ID {} not found.", id);
    }
}

void GraphExecutionSandboxSim::addConnection(Connection *connection)
{
    m_connections.push_back(connection);
}

void GraphExecutionSandboxSim::removeConnection(const std::string &id)
{
    m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(),
    [&id](Connection * connection) { return connection->getId() == id; }),
    m_connections.end());
}

bool GraphExecutionSandboxSim::configureStartupComponent(const std::string &componentId,
        const std::unordered_map<std::string, std::string> &properties)
{
    Component *component = getComponentById(componentId);

    if (!component)
    {
        LOGWF("[GraphExecutionSandboxSim][ERROR] Component with ID {} not found.", componentId);
        return false;
    }

    m_startupComponentId = componentId;
    m_startupProperties = properties;
    return true;
}

void GraphExecutionSandboxSim::clear()
{
    m_components.clear();
    m_connections.clear();
}

bool GraphExecutionSandboxSim::captureState()
{
    if (m_components.empty())
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] No components to capture state from.");
        return false;
    }

    if (m_connections.empty())
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] No connections to capture state from.");
        return false;
    }

    for (Component *component : m_components)
    {
        if (!component)
        {
            LOGW("[GraphExecutionSandboxSim][ERROR] Null component encountered during state capture.");
            return false;
        }

        std::vector<std::string> targetActorIds;

        for (Connection *connection : m_connections)
        {
            if (connection->getSourceComponentId() == component->getId())
            {
                targetActorIds.push_back(connection->getTargetComponentId());
            }
        }

        m_scheduler->registerActor(component->getId(), std::make_shared<ComponentActor>(component->getId(), component,
                                   m_scheduler, targetActorIds));
    }

    return true;
}

void GraphExecutionSandboxSim::stop()
{
    setRunning(false);
    m_scheduler->shutdown();
    LOGD("[GraphExecutionSandboxSim] Execution stopped.");
}

bool GraphExecutionSandboxSim::isRunning() const
{
    return m_isRunning;
}

QStringList GraphExecutionSandboxSim::getTimeline() const
{
    return m_timeline;
}

void GraphExecutionSandboxSim::execute()
{
    setRunning(true);

    if (!captureState())
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] Failed to capture state. Execution aborted.");
        setRunning(false);
        return;
    }

    // Send initial message to the startup component
    Component *startupComponent = getComponentById(m_startupComponentId);

    if (!startupComponent)
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] Startup component with ID {} not found.", m_startupComponentId);
        setRunning(false);
        return;
    }

    Message initialMessage{0, m_startupComponentId, m_startupProperties, startupComponent->snapshot()};
    m_scheduler->routeMessage(m_startupComponentId, std::move(initialMessage));
    m_scheduler->start();
}

void GraphExecutionSandboxSim::setRunning(bool running)
{
    if (m_isRunning != running)
    {
        m_isRunning = running;
        emit isRunningChanged();
    }
}
void GraphExecutionSandboxSim::prepareExecution()
{
    // TODO: Implement preparation logic before executing the graph simulation.
}

void GraphExecutionSandboxSim::commitState()
{
    // TODO: Implement logic to commit the state after executing the graph simulation.
}

void GraphExecutionSandboxSim::executeComponent(Component * component)
{
    if (!component)
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] Cannot execute a null component.");
        return;
    }

    LOGD("Executing component: ID={}, Type={}", component->getId(), component->getType());
    Tokens output;
    component->execute(output);
}
