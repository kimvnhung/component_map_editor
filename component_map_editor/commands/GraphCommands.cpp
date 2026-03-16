#include "GraphCommands.h"

// ---------------------------------------------------------------------------
// AddComponentCommand
// ---------------------------------------------------------------------------
AddComponentCommand::AddComponentCommand(GraphModel *graph, ComponentModel *component)
    : GraphCommand(QObject::tr("Add Component"))
    , m_graph(graph)
    , m_component(component)
    , m_owned(false)
{}

AddComponentCommand::~AddComponentCommand()
{
    if (m_owned)
        delete m_component;
}

void AddComponentCommand::redo()
{
    m_graph->addComponent(m_component);
    m_owned = false;
}

void AddComponentCommand::undo()
{
    m_graph->removeComponent(m_component->id());
    m_owned = true;
}

// ---------------------------------------------------------------------------
// RemoveComponentCommand
// ---------------------------------------------------------------------------
RemoveComponentCommand::RemoveComponentCommand(GraphModel *graph, const QString &componentId)
    : GraphCommand(QObject::tr("Remove Component"))
    , m_graph(graph)
    , m_componentId(componentId)
{}

RemoveComponentCommand::~RemoveComponentCommand()
{
    if (m_owned)
        delete m_component;
}

void RemoveComponentCommand::redo()
{
    m_component = m_graph->componentById(m_componentId);
    m_graph->removeComponent(m_componentId);
    m_owned = true;
}

void RemoveComponentCommand::undo()
{
    if (m_component) {
        m_graph->addComponent(m_component);
        m_owned = false;
    }
}

// ---------------------------------------------------------------------------
// MoveComponentCommand
// ---------------------------------------------------------------------------
MoveComponentCommand::MoveComponentCommand(GraphModel *graph, const QString &componentId,
                                           qreal oldX, qreal oldY, qreal newX, qreal newY)
    : GraphCommand(QObject::tr("Move Component"))
    , m_graph(graph)
    , m_componentId(componentId)
    , m_oldX(oldX), m_oldY(oldY)
    , m_newX(newX), m_newY(newY)
{}

void MoveComponentCommand::redo()
{
    if (ComponentModel *component = m_graph->componentById(m_componentId)) {
        component->setX(m_newX);
        component->setY(m_newY);
    }
}

void MoveComponentCommand::undo()
{
    if (ComponentModel *component = m_graph->componentById(m_componentId)) {
        component->setX(m_oldX);
        component->setY(m_oldY);
    }
}

bool MoveComponentCommand::mergeWith(const GraphCommand *newer)
{
    if (newer->id() != CommandId)
        return false;
    const auto *cmd = static_cast<const MoveComponentCommand *>(newer);
    if (cmd->m_componentId != m_componentId)
        return false;
    m_newX = cmd->m_newX;
    m_newY = cmd->m_newY;
    return true;
}

// ---------------------------------------------------------------------------
// AddConnectionCommand
// ---------------------------------------------------------------------------
AddConnectionCommand::AddConnectionCommand(GraphModel *graph, ConnectionModel *connection)
    : GraphCommand(QObject::tr("Add Connection"))
    , m_graph(graph)
    , m_connection(connection)
    , m_owned(false)
{}

AddConnectionCommand::~AddConnectionCommand()
{
    if (m_owned)
        delete m_connection;
}

void AddConnectionCommand::redo()
{
    m_graph->addConnection(m_connection);
    m_owned = false;
}

void AddConnectionCommand::undo()
{
    m_graph->removeConnection(m_connection->id());
    m_owned = true;
}

// ---------------------------------------------------------------------------
// RemoveConnectionCommand
// ---------------------------------------------------------------------------
RemoveConnectionCommand::RemoveConnectionCommand(GraphModel *graph, const QString &connectionId)
    : GraphCommand(QObject::tr("Remove Connection"))
    , m_graph(graph)
    , m_connectionId(connectionId)
{}

RemoveConnectionCommand::~RemoveConnectionCommand()
{
    if (m_owned)
        delete m_connection;
}

void RemoveConnectionCommand::redo()
{
    m_connection = m_graph->connectionById(m_connectionId);
    m_graph->removeConnection(m_connectionId);
    m_owned = true;
}

void RemoveConnectionCommand::undo()
{
    if (m_connection) {
        m_graph->addConnection(m_connection);
        m_owned = false;
    }
}

// ---------------------------------------------------------------------------
// SetConnectionSidesCommand
// ---------------------------------------------------------------------------
SetConnectionSidesCommand::SetConnectionSidesCommand(ConnectionModel *connection,
                                                     ConnectionModel::Side oldSourceSide,
                                                     ConnectionModel::Side oldTargetSide,
                                                     ConnectionModel::Side newSourceSide,
                                                     ConnectionModel::Side newTargetSide)
    : GraphCommand(QObject::tr("Update Connection Sides"))
    , m_connection(connection)
    , m_oldSourceSide(oldSourceSide)
    , m_oldTargetSide(oldTargetSide)
    , m_newSourceSide(newSourceSide)
    , m_newTargetSide(newTargetSide)
{}

void SetConnectionSidesCommand::redo()
{
    if (!m_connection)
        return;

    m_connection->setSourceSide(m_newSourceSide);
    m_connection->setTargetSide(m_newTargetSide);
}

void SetConnectionSidesCommand::undo()
{
    if (!m_connection)
        return;

    m_connection->setSourceSide(m_oldSourceSide);
    m_connection->setTargetSide(m_oldTargetSide);
}

