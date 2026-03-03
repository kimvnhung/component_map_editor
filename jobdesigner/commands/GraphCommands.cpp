#include "GraphCommands.h"

// ---------------------------------------------------------------------------
// AddNodeCommand
// ---------------------------------------------------------------------------
AddNodeCommand::AddNodeCommand(GraphModel *graph, NodeModel *node)
    : GraphCommand(QObject::tr("Add Node"))
    , m_graph(graph)
    , m_node(node)
    , m_owned(false)
{}

AddNodeCommand::~AddNodeCommand()
{
    if (m_owned)
        delete m_node;
}

void AddNodeCommand::redo()
{
    m_graph->addNode(m_node);
    m_owned = false;
}

void AddNodeCommand::undo()
{
    m_graph->removeNode(m_node->id());
    m_owned = true;
}

// ---------------------------------------------------------------------------
// RemoveNodeCommand
// ---------------------------------------------------------------------------
RemoveNodeCommand::RemoveNodeCommand(GraphModel *graph, const QString &nodeId)
    : GraphCommand(QObject::tr("Remove Node"))
    , m_graph(graph)
    , m_nodeId(nodeId)
{}

RemoveNodeCommand::~RemoveNodeCommand()
{
    if (m_owned)
        delete m_node;
}

void RemoveNodeCommand::redo()
{
    m_node = m_graph->nodeById(m_nodeId);
    m_graph->removeNode(m_nodeId);
    m_owned = true;
}

void RemoveNodeCommand::undo()
{
    if (m_node) {
        m_graph->addNode(m_node);
        m_owned = false;
    }
}

// ---------------------------------------------------------------------------
// MoveNodeCommand
// ---------------------------------------------------------------------------
MoveNodeCommand::MoveNodeCommand(GraphModel *graph, const QString &nodeId,
                                 qreal oldX, qreal oldY, qreal newX, qreal newY)
    : GraphCommand(QObject::tr("Move Node"))
    , m_graph(graph)
    , m_nodeId(nodeId)
    , m_oldX(oldX), m_oldY(oldY)
    , m_newX(newX), m_newY(newY)
{}

void MoveNodeCommand::redo()
{
    if (NodeModel *node = m_graph->nodeById(m_nodeId)) {
        node->setX(m_newX);
        node->setY(m_newY);
    }
}

void MoveNodeCommand::undo()
{
    if (NodeModel *node = m_graph->nodeById(m_nodeId)) {
        node->setX(m_oldX);
        node->setY(m_oldY);
    }
}

bool MoveNodeCommand::mergeWith(const GraphCommand *newer)
{
    if (newer->id() != CommandId)
        return false;
    const auto *cmd = static_cast<const MoveNodeCommand *>(newer);
    if (cmd->m_nodeId != m_nodeId)
        return false;
    m_newX = cmd->m_newX;
    m_newY = cmd->m_newY;
    return true;
}

// ---------------------------------------------------------------------------
// AddEdgeCommand
// ---------------------------------------------------------------------------
AddEdgeCommand::AddEdgeCommand(GraphModel *graph, EdgeModel *edge)
    : GraphCommand(QObject::tr("Add Edge"))
    , m_graph(graph)
    , m_edge(edge)
    , m_owned(false)
{}

AddEdgeCommand::~AddEdgeCommand()
{
    if (m_owned)
        delete m_edge;
}

void AddEdgeCommand::redo()
{
    m_graph->addEdge(m_edge);
    m_owned = false;
}

void AddEdgeCommand::undo()
{
    m_graph->removeEdge(m_edge->id());
    m_owned = true;
}

// ---------------------------------------------------------------------------
// RemoveEdgeCommand
// ---------------------------------------------------------------------------
RemoveEdgeCommand::RemoveEdgeCommand(GraphModel *graph, const QString &edgeId)
    : GraphCommand(QObject::tr("Remove Edge"))
    , m_graph(graph)
    , m_edgeId(edgeId)
{}

RemoveEdgeCommand::~RemoveEdgeCommand()
{
    if (m_owned)
        delete m_edge;
}

void RemoveEdgeCommand::redo()
{
    m_edge = m_graph->edgeById(m_edgeId);
    m_graph->removeEdge(m_edgeId);
    m_owned = true;
}

void RemoveEdgeCommand::undo()
{
    if (m_edge) {
        m_graph->addEdge(m_edge);
        m_owned = false;
    }
}

