#ifndef GRAPHCOMMANDS_H
#define GRAPHCOMMANDS_H

#include <QPointF>

#include "GraphCommand.h"
#include "models/GraphModel.h"
#include "models/NodeModel.h"
#include "models/EdgeModel.h"

// ---------------------------------------------------------------------------
// AddNodeCommand
// ---------------------------------------------------------------------------
class AddNodeCommand : public GraphCommand
{
public:
    AddNodeCommand(GraphModel *graph, NodeModel *node);
    ~AddNodeCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel *m_graph;
    NodeModel  *m_node;
    bool        m_owned = false; // true while the command owns the node
};

// ---------------------------------------------------------------------------
// RemoveNodeCommand
// ---------------------------------------------------------------------------
class RemoveNodeCommand : public GraphCommand
{
public:
    RemoveNodeCommand(GraphModel *graph, const QString &nodeId);
    ~RemoveNodeCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel *m_graph;
    QString     m_nodeId;
    NodeModel  *m_node   = nullptr;
    bool        m_owned  = false;
};

// ---------------------------------------------------------------------------
// MoveNodeCommand — mergeable so that continuous drags collapse into one step
// ---------------------------------------------------------------------------
class MoveNodeCommand : public GraphCommand
{
public:
    static constexpr int CommandId = 1001;

    MoveNodeCommand(GraphModel *graph, const QString &nodeId,
                    qreal oldX, qreal oldY, qreal newX, qreal newY);

    void undo() override;
    void redo() override;
    bool mergeWith(const GraphCommand *newer) override;
    int  id() const override { return CommandId; }

private:
    GraphModel *m_graph;
    QString     m_nodeId;
    qreal       m_oldX, m_oldY;
    qreal       m_newX, m_newY;
};

// ---------------------------------------------------------------------------
// AddEdgeCommand
// ---------------------------------------------------------------------------
class AddEdgeCommand : public GraphCommand
{
public:
    AddEdgeCommand(GraphModel *graph, EdgeModel *edge);
    ~AddEdgeCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel *m_graph;
    EdgeModel  *m_edge;
    bool        m_owned = false;
};

// ---------------------------------------------------------------------------
// RemoveEdgeCommand
// ---------------------------------------------------------------------------
class RemoveEdgeCommand : public GraphCommand
{
public:
    RemoveEdgeCommand(GraphModel *graph, const QString &edgeId);
    ~RemoveEdgeCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel *m_graph;
    QString     m_edgeId;
    EdgeModel  *m_edge  = nullptr;
    bool        m_owned = false;
};

#endif // GRAPHCOMMANDS_H
