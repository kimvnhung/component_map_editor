#ifndef GRAPHCOMMANDS_H
#define GRAPHCOMMANDS_H

#include <QPointF>
#include <QPointer>
#include <QVariant>

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
    QPointer<ConnectionModel> m_connection;
    ConnectionModel::Side m_oldSourceSide;
    ConnectionModel::Side m_oldTargetSide;
    ConnectionModel::Side m_newSourceSide;
    ConnectionModel::Side m_newTargetSide;
};

// ---------------------------------------------------------------------------
// RemoveComponentWithConnectionsCommand
// ---------------------------------------------------------------------------
class RemoveComponentWithConnectionsCommand : public GraphCommand
{
public:
    RemoveComponentWithConnectionsCommand(GraphModel *graph, const QString &componentId);
    ~RemoveComponentWithConnectionsCommand() override;
    void undo() override;
    void redo() override;

private:
    GraphModel *m_graph;
    QString m_componentId;
    ComponentModel *m_component = nullptr;
    QList<QString> m_connectionIds;
    QList<ConnectionModel *> m_connections;
    bool m_componentOwned = false;
    bool m_connectionsOwned = false;
};

// ---------------------------------------------------------------------------
// MoveComponentsCommand
// ---------------------------------------------------------------------------
class MoveComponentsCommand : public GraphCommand
{
public:
    static constexpr int CommandId = 1002;

    struct MoveEntry {
        QString componentId;
        qreal oldX = 0.0;
        qreal oldY = 0.0;
        qreal newX = 0.0;
        qreal newY = 0.0;
    };

    MoveComponentsCommand(GraphModel *graph, const QList<MoveEntry> &entries);

    void undo() override;
    void redo() override;
    bool mergeWith(const GraphCommand *newer) override;
    int id() const override { return CommandId; }

private:
    GraphModel *m_graph;
    QList<MoveEntry> m_entries;
};

// ---------------------------------------------------------------------------
// SetComponentGeometryCommand
// ---------------------------------------------------------------------------
class SetComponentGeometryCommand : public GraphCommand
{
public:
    static constexpr int CommandId = 1003;

    SetComponentGeometryCommand(ComponentModel *component,
                                qreal oldX,
                                qreal oldY,
                                qreal oldWidth,
                                qreal oldHeight,
                                qreal newX,
                                qreal newY,
                                qreal newWidth,
                                qreal newHeight);

    void undo() override;
    void redo() override;
    bool mergeWith(const GraphCommand *newer) override;
    int id() const override { return CommandId; }

private:
    QPointer<ComponentModel> m_component;
    qreal m_oldX;
    qreal m_oldY;
    qreal m_oldWidth;
    qreal m_oldHeight;
    qreal m_newX;
    qreal m_newY;
    qreal m_newWidth;
    qreal m_newHeight;
};

// ---------------------------------------------------------------------------
// SetComponentPropertyCommand
// ---------------------------------------------------------------------------
class SetComponentPropertyCommand : public GraphCommand
{
public:
    static constexpr int CommandId = 2001;

    SetComponentPropertyCommand(ComponentModel *component,
                                const QByteArray &propertyName,
                                const QVariant &oldValue,
                                const QVariant &newValue);

    void undo() override;
    void redo() override;
    bool mergeWith(const GraphCommand *newer) override;
    int id() const override { return CommandId; }

private:
    QPointer<ComponentModel> m_component;
    QByteArray m_propertyName;
    QVariant m_oldValue;
    QVariant m_newValue;
};

// ---------------------------------------------------------------------------
// SetConnectionPropertyCommand
// ---------------------------------------------------------------------------
class SetConnectionPropertyCommand : public GraphCommand
{
public:
    static constexpr int CommandId = 2002;

    SetConnectionPropertyCommand(ConnectionModel *connection,
                                 const QByteArray &propertyName,
                                 const QVariant &oldValue,
                                 const QVariant &newValue);

    void undo() override;
    void redo() override;
    bool mergeWith(const GraphCommand *newer) override;
    int id() const override { return CommandId; }

private:
    QPointer<ConnectionModel> m_connection;
    QByteArray m_propertyName;
    QVariant m_oldValue;
    QVariant m_newValue;
};

#endif // GRAPHCOMMANDS_H
