#ifndef GRAPHCOMMANDS_H
#define GRAPHCOMMANDS_H

#include <QPointF>

#include "GraphCommand.h"
#include "models/GraphModel.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"

// ---------------------------------------------------------------------------
// AddComponentCommand
// ---------------------------------------------------------------------------
class AddComponentCommand : public GraphCommand
{
public:
    AddComponentCommand(GraphModel *graph, ComponentModel *component);
    ~AddComponentCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel *m_graph;
    ComponentModel  *m_component;
    bool             m_owned = false; // true while the command owns the component
};

// ---------------------------------------------------------------------------
// RemoveComponentCommand
// ---------------------------------------------------------------------------
class RemoveComponentCommand : public GraphCommand
{
public:
    RemoveComponentCommand(GraphModel *graph, const QString &componentId);
    ~RemoveComponentCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel     *m_graph;
    QString         m_componentId;
    ComponentModel *m_component = nullptr;
    bool            m_owned = false;
};

// ---------------------------------------------------------------------------
// MoveComponentCommand — mergeable so that continuous drags collapse into one step
// ---------------------------------------------------------------------------
class MoveComponentCommand : public GraphCommand
{
public:
    static constexpr int CommandId = 1001;

    MoveComponentCommand(GraphModel *graph, const QString &componentId,
                         qreal oldX, qreal oldY, qreal newX, qreal newY);

    void undo() override;
    void redo() override;
    bool mergeWith(const GraphCommand *newer) override;
    int  id() const override { return CommandId; }

private:
    GraphModel *m_graph;
    QString     m_componentId;
    qreal       m_oldX, m_oldY;
    qreal       m_newX, m_newY;
};

// ---------------------------------------------------------------------------
// AddConnectionCommand
// ---------------------------------------------------------------------------
class AddConnectionCommand : public GraphCommand
{
public:
    AddConnectionCommand(GraphModel *graph, ConnectionModel *connection);
    ~AddConnectionCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel      *m_graph;
    ConnectionModel *m_connection;
    bool             m_owned = false;
};

// ---------------------------------------------------------------------------
// RemoveConnectionCommand
// ---------------------------------------------------------------------------
class RemoveConnectionCommand : public GraphCommand
{
public:
    RemoveConnectionCommand(GraphModel *graph, const QString &connectionId);
    ~RemoveConnectionCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel      *m_graph;
    QString          m_connectionId;
    ConnectionModel *m_connection = nullptr;
    bool             m_owned = false;
};

// ---------------------------------------------------------------------------
// SetConnectionSidesCommand
// ---------------------------------------------------------------------------
class SetConnectionSidesCommand : public GraphCommand
{
public:
    SetConnectionSidesCommand(ConnectionModel *connection,
                              ConnectionModel::Side oldSourceSide,
                              ConnectionModel::Side oldTargetSide,
                              ConnectionModel::Side newSourceSide,
                              ConnectionModel::Side newTargetSide);

    void undo() override;
    void redo() override;

private:
    ConnectionModel *m_connection;
    ConnectionModel::Side m_oldSourceSide;
    ConnectionModel::Side m_oldTargetSide;
    ConnectionModel::Side m_newSourceSide;
    ConnectionModel::Side m_newTargetSide;
};

#endif // GRAPHCOMMANDS_H
