#include "GraphExecutionSandboxSim.h"

#include <QThread>
#include <base_log.h>

#include "Actor.h"
#include "ActorSystem.h"
#include "Message.h"

std::string new_id(GraphItemType type)
{
    static int componentCounter = 0;
    static int connectionCounter = 0;
    return std::to_string(static_cast<int>(type)) + "_" + std::to_string((type == Component_Type ? componentCounter++ :
            connectionCounter++));
}

Component::Component(const std::string &id, const std::string &type)
    : m_id(id), m_type(type) {}

std::string Component::getId() const { return m_id; }
std::string Component::getType() const { return m_type; }
void Component::addProperty(const std::string &key, const std::string &value) { m_properties[key] = value; }
void Component::removeProperty(const std::string &key) { m_properties.erase(key); }
std::string Component::getProperty(const std::string &key) const
{
    auto it = m_properties.find(key);
    return (it != m_properties.end()) ? it->second : "";
}

Connection::Connection(const std::string &id, const std::string &sourceComponentId,
                       const std::string &targetComponentId)
    : m_id(id), m_sourceComponentId(sourceComponentId), m_targetComponentId(targetComponentId) {}
std::string Connection::getId() const { return m_id; }
std::string Connection::getSourceComponentId() const { return m_sourceComponentId; }
std::string Connection::getTargetComponentId() const { return m_targetComponentId; }
void Connection::setSourceComponentId(const std::string &sourceComponentId) { m_sourceComponentId = sourceComponentId; }
void Connection::setTargetComponentId(const std::string &targetComponentId) { m_targetComponentId = targetComponentId; }
void Connection::addProperty(const std::string &key, const std::string &value) { m_properties[key] = value; }
void Connection::removeProperty(const std::string &key) { m_properties.erase(key); }
std::string Connection::getProperty(const std::string &key) const
{
    auto it = m_properties.find(key);
    return (it != m_properties.end()) ? it->second : "";
}

ClockComponent::ClockComponent(const std::string &id)
    : Component(id, "Clock") {}

bool ClockComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    int tick = 0;

    if (inputTokens.find("tick") != inputTokens.end())
    {
        std::string tickValue = inputTokens.at("tick");
        tick = std::stoi(tickValue);
    }

    QThread::msleep(1000); // Simulate a clock tick every second
    outputTokens["tick"] = std::to_string(tick + 1);
    return true;
}

PrinterComponent::PrinterComponent(const std::string &id)
    : Component(id, "Printer") {}

bool PrinterComponent::execute(Tokens &outputTokens, const Tokens &inputTokens, const Tokens &componentSnapshot)
{
    if (inputTokens.find("tick") != inputTokens.end())
    {
        std::string tickValue = inputTokens.at("tick");
        LOGDF("PrinterComponent received tick: {}", tickValue);
        outputTokens["printed"] = "Printed tick: " + tickValue;
        outputTokens["tick"] = tickValue;
        return true;
    }

    return false;
}

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
        LOGD("Registered actor for component ID: {}", component->getId());
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


    m_actorSystem->send(componentId, new Message(properties)); // Send initial properties to the startup component
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
    if (result.status == ActorTaskResult::Status::Success && result.outputMessage)
    {
        std::string actorId = result.outputMessage->getActorId();
        Tokens outputTokens = result.outputMessage->getTokens();

        // Route tokens to outgoing connections
        for (Connection *connection : getOutgoingConnections(actorId))
        {
            Component *targetComponent = getComponentById(connection->getTargetComponentId());

            if (targetComponent)
            {
                m_actorSystem->send(targetComponent->getId(), new Message(outputTokens, targetComponent->snapshot()));
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

    delete result.outputMessage; // Clean up the output message
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
