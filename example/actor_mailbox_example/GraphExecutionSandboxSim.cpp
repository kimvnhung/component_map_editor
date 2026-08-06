#include "GraphExecutionSandboxSim.h"

#include <QThread>
#include <base_log.h>

#include "Actor.h"
#include "ActorSystem.h"
#include "Message.h"



GraphExecutionSandboxSim::GraphExecutionSandboxSim()
    : m_actorSystem(new ActorSystem())
    , m_scheduler(new ActorScheduler())
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

    if (m_actorSystem->registerActor(component))
    {
        LOGDF("Registered actor for component ID: {}", component->getId());
    }
    else
    {
        LOGWF("[GraphExecutionSandboxSim][ERROR] Failed to register actor for component ID: {}", component->getId());
    }
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

        m_actorSystem->unregisterActor(id);
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


    m_actorSystem->send(componentId, Message(properties)); // Send initial properties to the startup component
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

    if (!m_connections.empty())
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] Connections should be empty when capturing state.");
        return false;
    }

    return true;
}

void GraphExecutionSandboxSim::stop()
{
    setRunning(false);
    m_actorSystem->stop();
    LOGD("[GraphExecutionSandboxSim] Execution stopped.");
}

void GraphExecutionSandboxSim::routeTokens(const ActorTaskResult &result)
{
    if (result.status == ActorTaskResult::Status::Success)
    {
        std::string actorId = result.outputMessage.getActorId();
        Tokens outputTokens = result.outputMessage.getTokens();

        // Route tokens to outgoing connections
        for (Connection *connection : getOutgoingConnections(actorId))
        {
            Component *targetComponent = getComponentById(connection->getTargetComponentId());

            if (targetComponent)
            {
                Message message(outputTokens, targetComponent->snapshot());
                m_actorSystem->send(targetComponent->getId(), std::move(message));
            }
            else
            {
                LOGWF("[GraphExecutionSandboxSim][ERROR] Target component with ID {} not found for connection {}.",
                      connection->getTargetComponentId(), connection->getId());
            }
        }
    }
    else if (result.status == ActorTaskResult::Status::Failure)
    {
        LOGWF("[GraphExecutionSandboxSim][ERROR] Task failed: {}", result.error);
        stop();
    }
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
    QThreadPool::globalInstance()->start([this]()
    {
        while (m_isRunning)
        {
            Actor *nextActor = m_actorSystem->nextReadyActor();

            if (nextActor)
            {
                ActorTask task =
                {
                    std::bind(&Actor::ProcessNextMessage, nextActor),
                    std::bind(&GraphExecutionSandboxSim::routeTokens, this, std::placeholders::_1)
                };
                m_scheduler->scheduleTask(task);
            }
            else
            {
                LOGD("[GraphExecutionSandboxSim] No ready actors. Waiting for messages...");
            }
        }

        LOGD("[GraphExecutionSandboxSim] Execution completed.");
    });
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
