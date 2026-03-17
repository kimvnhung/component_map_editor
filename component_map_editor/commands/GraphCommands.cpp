#include "GraphCommands.h"

#include <QtMath>

namespace {

void applyComponentGeometry(ComponentModel *component,
                           qreal x,
                           qreal y,
                           qreal width,
                           qreal height)
{
    if (!component)
        return;
    component->setX(x);
    component->setY(y);
    component->setWidth(width);
    component->setHeight(height);
}

} // namespace

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

// ---------------------------------------------------------------------------
// RemoveComponentWithConnectionsCommand
// ---------------------------------------------------------------------------
RemoveComponentWithConnectionsCommand::RemoveComponentWithConnectionsCommand(GraphModel *graph,
                                                                             const QString &componentId)
    : GraphCommand(QObject::tr("Remove Component"))
    , m_graph(graph)
    , m_componentId(componentId)
{}

RemoveComponentWithConnectionsCommand::~RemoveComponentWithConnectionsCommand()
{
    if (m_connectionsOwned)
        qDeleteAll(m_connections);
    if (m_componentOwned)
        delete m_component;
}

void RemoveComponentWithConnectionsCommand::redo()
{
    if (!m_graph)
        return;

    if (!m_component)
        m_component = m_graph->componentById(m_componentId);
    if (!m_component)
        return;

    if (m_connections.isEmpty()) {
        const QList<ConnectionModel *> existingConnections = m_graph->connectionList();
        for (ConnectionModel *connection : existingConnections) {
            if (!connection)
                continue;
            if (connection->sourceId() == m_componentId || connection->targetId() == m_componentId) {
                m_connectionIds.append(connection->id());
                m_connections.append(connection);
            }
        }
    }

    for (const QString &connectionId : std::as_const(m_connectionIds))
        m_graph->removeConnection(connectionId);
    m_connectionsOwned = true;

    m_graph->removeComponent(m_componentId);
    m_componentOwned = true;
}

void RemoveComponentWithConnectionsCommand::undo()
{
    if (!m_graph || !m_component)
        return;

    m_graph->addComponent(m_component);
    m_componentOwned = false;

    for (ConnectionModel *connection : std::as_const(m_connections)) {
        if (connection)
            m_graph->addConnection(connection);
    }
    m_connectionsOwned = false;
}

// ---------------------------------------------------------------------------
// MoveComponentsCommand
// ---------------------------------------------------------------------------
MoveComponentsCommand::MoveComponentsCommand(GraphModel *graph,
                                             const QList<MoveEntry> &entries)
    : GraphCommand(QObject::tr("Move Components"))
    , m_graph(graph)
    , m_entries(entries)
{}

void MoveComponentsCommand::redo()
{
    if (!m_graph)
        return;

    for (const MoveEntry &entry : std::as_const(m_entries)) {
        if (ComponentModel *component = m_graph->componentById(entry.componentId)) {
            component->setX(entry.newX);
            component->setY(entry.newY);
        }
    }
}

void MoveComponentsCommand::undo()
{
    if (!m_graph)
        return;

    for (const MoveEntry &entry : std::as_const(m_entries)) {
        if (ComponentModel *component = m_graph->componentById(entry.componentId)) {
            component->setX(entry.oldX);
            component->setY(entry.oldY);
        }
    }
}

bool MoveComponentsCommand::mergeWith(const GraphCommand *newer)
{
    if (newer->id() != CommandId)
        return false;

    const auto *cmd = static_cast<const MoveComponentsCommand *>(newer);
    if (cmd->m_entries.size() != m_entries.size())
        return false;

    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].componentId != cmd->m_entries[i].componentId)
            return false;
    }

    for (int i = 0; i < m_entries.size(); ++i) {
        m_entries[i].newX = cmd->m_entries[i].newX;
        m_entries[i].newY = cmd->m_entries[i].newY;
    }
    return true;
}

// ---------------------------------------------------------------------------
// SetComponentGeometryCommand
// ---------------------------------------------------------------------------
SetComponentGeometryCommand::SetComponentGeometryCommand(ComponentModel *component,
                                                         qreal oldX,
                                                         qreal oldY,
                                                         qreal oldWidth,
                                                         qreal oldHeight,
                                                         qreal newX,
                                                         qreal newY,
                                                         qreal newWidth,
                                                         qreal newHeight)
    : GraphCommand(QObject::tr("Resize Component"))
    , m_component(component)
    , m_oldX(oldX)
    , m_oldY(oldY)
    , m_oldWidth(oldWidth)
    , m_oldHeight(oldHeight)
    , m_newX(newX)
    , m_newY(newY)
    , m_newWidth(newWidth)
    , m_newHeight(newHeight)
{}

void SetComponentGeometryCommand::redo()
{
    applyComponentGeometry(m_component, m_newX, m_newY, m_newWidth, m_newHeight);
}

void SetComponentGeometryCommand::undo()
{
    applyComponentGeometry(m_component, m_oldX, m_oldY, m_oldWidth, m_oldHeight);
}

bool SetComponentGeometryCommand::mergeWith(const GraphCommand *newer)
{
    if (newer->id() != CommandId)
        return false;
    const auto *cmd = static_cast<const SetComponentGeometryCommand *>(newer);
    if (cmd->m_component != m_component)
        return false;

    m_newX = cmd->m_newX;
    m_newY = cmd->m_newY;
    m_newWidth = cmd->m_newWidth;
    m_newHeight = cmd->m_newHeight;
    return true;
}

// ---------------------------------------------------------------------------
// SetComponentPropertyCommand
// ---------------------------------------------------------------------------
SetComponentPropertyCommand::SetComponentPropertyCommand(ComponentModel *component,
                                                         const QByteArray &propertyName,
                                                         const QVariant &oldValue,
                                                         const QVariant &newValue)
    : GraphCommand(QObject::tr("Edit Component"))
    , m_component(component)
    , m_propertyName(propertyName)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
{}

void SetComponentPropertyCommand::redo()
{
    if (!m_component)
        return;
    m_component->setProperty(m_propertyName.constData(), m_newValue);
}

void SetComponentPropertyCommand::undo()
{
    if (!m_component)
        return;
    m_component->setProperty(m_propertyName.constData(), m_oldValue);
}

bool SetComponentPropertyCommand::mergeWith(const GraphCommand *newer)
{
    if (newer->id() != CommandId)
        return false;
    const auto *cmd = static_cast<const SetComponentPropertyCommand *>(newer);
    if (cmd->m_component != m_component || cmd->m_propertyName != m_propertyName)
        return false;

    m_newValue = cmd->m_newValue;
    return true;
}

// ---------------------------------------------------------------------------
// SetConnectionPropertyCommand
// ---------------------------------------------------------------------------
SetConnectionPropertyCommand::SetConnectionPropertyCommand(ConnectionModel *connection,
                                                           const QByteArray &propertyName,
                                                           const QVariant &oldValue,
                                                           const QVariant &newValue)
    : GraphCommand(QObject::tr("Edit Connection"))
    , m_connection(connection)
    , m_propertyName(propertyName)
    , m_oldValue(oldValue)
    , m_newValue(newValue)
{}

void SetConnectionPropertyCommand::redo()
{
    if (!m_connection)
        return;
    m_connection->setProperty(m_propertyName.constData(), m_newValue);
}

void SetConnectionPropertyCommand::undo()
{
    if (!m_connection)
        return;
    m_connection->setProperty(m_propertyName.constData(), m_oldValue);
}

bool SetConnectionPropertyCommand::mergeWith(const GraphCommand *newer)
{
    if (newer->id() != CommandId)
        return false;
    const auto *cmd = static_cast<const SetConnectionPropertyCommand *>(newer);
    if (cmd->m_connection != m_connection || cmd->m_propertyName != m_propertyName)
        return false;

    m_newValue = cmd->m_newValue;
    return true;
}

