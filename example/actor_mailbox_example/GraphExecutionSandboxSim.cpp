#include "GraphExecutionSandboxSim.h"

#include <QThread>
#include <base_log.h>

#include "Actor.h"

GraphExecutionSandboxSim::GraphExecutionSandboxSim()
    : m_scheduler(new ActorScheduler(std::bind(&GraphExecutionSandboxSim::onStepCompleted, this, std::placeholders::_1,
                                     std::placeholders::_2)))
    , m_stateCapture(new ExecutionStateCapture())
{}

std::vector<Component *> GraphExecutionSandboxSim::getComponents() const
{
    return m_components;
}

std::vector<Connection *> GraphExecutionSandboxSim::getConnections() const
{
    return m_connections;
}

Component *GraphExecutionSandboxSim::getComponentById(const QString &id) const
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

Connection *GraphExecutionSandboxSim::getConnectionById(const QString &id) const
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


std::vector<Connection *> GraphExecutionSandboxSim::getIncomingConnections(const QString &componentId) const
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

std::vector<Connection *> GraphExecutionSandboxSim::getOutgoingConnections(const QString &componentId) const
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

void GraphExecutionSandboxSim::removeComponent(const QString &id)
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
        LOGWF("[GraphExecutionSandboxSim][ERROR] Component with ID {} not found.", id.toStdString());
    }
}

void GraphExecutionSandboxSim::addConnection(Connection *connection)
{
    m_connections.push_back(connection);
}

void GraphExecutionSandboxSim::removeConnection(const QString &id)
{
    m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(),
    [&id](Connection * connection) { return connection->getId() == id; }),
    m_connections.end());
}

bool GraphExecutionSandboxSim::configureStartupComponent(const QString &componentId,
        const QVariantMap &properties)
{
    Component *component = getComponentById(componentId);

    if (!component)
    {
        LOGWF("[GraphExecutionSandboxSim][ERROR] Component with ID {} not found.", componentId.toStdString());
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

void GraphExecutionSandboxSim::onStepCompleted(const ExecutionContext& ctx, const ExecuteResult& result)
{
    if (result.success)
    {
        LOGDF("[GraphExecutionSandboxSim] Step completed for component {}. Output tokens: {}", ctx.componentId.toStdString(),
              token2string(result.outputState).toStdString());

        if (getExecutionStatus() == ExecutionStatus::STEPPING)
        {
            LOGD("[GraphExecutionSandboxSim] Execution is in STEPPING mode. Storing last context and result for next step.");
            m_lastCtx = ctx;
            m_lastResult = result;
        }
        else
        {
            // Clear last context and result after successful execution in RUNNING mode
            m_lastCtx = {};
            m_lastResult = {};

            if (getExecutionStatus() == ExecutionStatus::RUNNING)
            {
                Message msg{0, ctx.componentId, result.outputState};
                m_scheduler->routeMessage(std::move(msg));
            }
            else if (getExecutionStatus() == ExecutionStatus::STEPPING)
            {
                setExecutionStatus(ExecutionStatus::PAUSED);
                LOGD("[GraphExecutionSandboxSim] Execution is stepping. Transitioning to PAUSED after this step.");
            }
            else
            {
                LOGWF("[GraphExecutionSandboxSim] Execution status is {}, not routing further messages.",
                      static_cast<int>(getExecutionStatus()));
            }
        }
    }
    else
    {
        LOGWF("[GraphExecutionSandboxSim] Step failed for component {}.", ctx.componentId.toStdString());
        // TODO: Need handle more
    }
}

QVariantMap GraphExecutionSandboxSim::componentSnapshot(const QString &componentId) const
{
    Component *component = getComponentById(componentId);

    if (!component)
    {
        LOGWF("[GraphExecutionSandboxSim][ERROR] Component with ID {} not found for snapshot.", componentId.toStdString());
        return {};
    }

    return component->snapshot();
}

GraphExecutionSandboxSim::ExecutionStatus GraphExecutionSandboxSim::getExecutionStatus() const
{
    return m_executionStatus;
}

void GraphExecutionSandboxSim::setExecutionStatus(ExecutionStatus status)
{
    if (m_executionStatus != status)
    {
        m_executionStatus = status;
        emit executionStatusChanged();
    }
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

        m_scheduler->registerActor(component->getId(), std::make_shared<ComponentActor>(component->getId(), component,
                                   m_scheduler));
    }

    return true;
}

void GraphExecutionSandboxSim::pause()
{
    setExecutionStatus(ExecutionStatus::PAUSED);
    m_scheduler->setExecutionMode(ActorScheduler::ExecutionMode::SEQUENTIAL);
}

void GraphExecutionSandboxSim::step()
{
    setExecutionStatus(ExecutionStatus::STEPPING);
    static uint64_t stepCounter = 0;
    m_scheduler->setExecutionMode(ActorScheduler::ExecutionMode::SEQUENTIAL);

    if (stepCounter == 0)
    {
        Message initialMessage{stepCounter++, "", m_startupProperties};
        m_scheduler->routeMessageToActor(m_startupComponentId, std::move(initialMessage));
    }
    else if (m_lastCtx.componentId != "")
    {
        Message msg{stepCounter++, m_lastCtx.componentId, m_lastResult.outputState};
        m_scheduler->routeMessage(std::move(msg));
    }
    else
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] No last context available for stepping.");
        setExecutionStatus(ExecutionStatus::ERROR);
    }

    LOGDF("[GraphExecutionSandboxSim] Step {} executed. Current execution status: {}", stepCounter,
          static_cast<int>(getExecutionStatus()));
}


QStringList GraphExecutionSandboxSim::getTimeline() const
{
    return m_timeline;
}

void GraphExecutionSandboxSim::reset()
{
    m_timeline.clear();
    setExecutionStatus(ExecutionStatus::COMPLETED);
    m_scheduler->shutdown();
    LOGD("[GraphExecutionSandboxSim] Execution state reset.");
}

void GraphExecutionSandboxSim::run()
{
    m_scheduler->setExecutionMode(ActorScheduler::ExecutionMode::PARALLEL);
}

void GraphExecutionSandboxSim::start()
{
    setExecutionStatus(ExecutionStatus::RUNNING);

    if (!captureState())
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] Failed to capture state. Execution aborted.");
        setExecutionStatus(ExecutionStatus::ERROR);
        return;
    }

    // Send initial message to the startup component
    Component *startupComponent = getComponentById(m_startupComponentId);

    if (!startupComponent)
    {
        LOGW("[GraphExecutionSandboxSim][ERROR] Startup component with ID {} not found.", m_startupComponentId.toStdString());
        setExecutionStatus(ExecutionStatus::ERROR);
        return;
    }

    auto routeTable = ConnectionRoutingTable::buildFromGraphSnapshot(m_components, m_connections);
    m_scheduler->start(std::move(routeTable));
}