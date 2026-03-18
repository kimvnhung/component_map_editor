#include "UndoStack.h"

#include "GraphCommands.h"

#include <QSet>

namespace {

ConnectionModel::Side sideFromInt(int sideValue)
{
    switch (sideValue) {
    case ConnectionModel::SideTop:
    case ConnectionModel::SideRight:
    case ConnectionModel::SideBottom:
    case ConnectionModel::SideLeft:
    case ConnectionModel::SideAuto:
        return static_cast<ConnectionModel::Side>(sideValue);
    default:
        return ConnectionModel::SideAuto;
    }
}

bool isConnectionPropertyUndoable(const QString &name)
{
    static const QSet<QString> kAllowed {
        QStringLiteral("id"),
        QStringLiteral("sourceId"),
        QStringLiteral("targetId"),
        QStringLiteral("label"),
        QStringLiteral("sourceSide"),
        QStringLiteral("targetSide")
    };
    return kAllowed.contains(name);
}

} // namespace

UndoStack::UndoStack(QObject *parent)
    : QObject(parent)
{}

UndoStack::~UndoStack()
{
    qDeleteAll(m_commands);
}

bool UndoStack::canUndo() const { return m_index >= 0; }
bool UndoStack::canRedo() const { return m_index < m_commands.size() - 1; }

QString UndoStack::undoText() const
{
    return canUndo() ? m_commands.at(m_index)->text() : QString();
}

QString UndoStack::redoText() const
{
    return canRedo() ? m_commands.at(m_index + 1)->text() : QString();
}

int UndoStack::count() const { return m_commands.size(); }

void UndoStack::push(GraphCommand *command)
{
    if (!command)
        return;

    const bool prevCanUndo = canUndo();
    const bool prevCanRedo = canRedo();
    const QString prevUndoText = undoText();
    const QString prevRedoText = redoText();
    const int prevCount = count();

    // Discard any commands above the current index (redo history).
    while (m_commands.size() > m_index + 1) {
        delete m_commands.takeLast();
    }

    // Try to merge with the topmost command.
    if (canUndo() && m_commands.last()->id() != -1
        && m_commands.last()->id() == command->id()
        && m_commands.last()->mergeWith(command)) {
        // Apply merged latest state immediately so command-driven edits stay live.
        m_commands.last()->redo();
        delete command;
    } else {
        m_commands.append(command);
        ++m_index;
        command->redo();
    }

    notifyChanges(prevCanUndo, prevCanRedo, prevUndoText, prevRedoText, prevCount);
}

void UndoStack::pushAddComponent(GraphModel *graph, ComponentModel *component)
{
    if (!graph || !component)
        return;

    push(new AddComponentCommand(graph, component));
}

void UndoStack::pushRemoveComponent(GraphModel *graph, const QString &componentId)
{
    if (!graph || componentId.isEmpty())
        return;

    push(new RemoveComponentWithConnectionsCommand(graph, componentId));
}

void UndoStack::pushMoveComponent(GraphModel *graph,
                                  const QString &componentId,
                                  qreal oldX,
                                  qreal oldY,
                                  qreal newX,
                                  qreal newY)
{
    if (!graph || componentId.isEmpty())
        return;
    if (qFuzzyCompare(oldX, newX) && qFuzzyCompare(oldY, newY))
        return;

    push(new MoveComponentCommand(graph, componentId, oldX, oldY, newX, newY));
}

void UndoStack::pushMoveComponents(GraphModel *graph, const QVariantList &moves)
{
    if (!graph || moves.isEmpty())
        return;

    QList<MoveComponentsCommand::MoveEntry> entries;
    entries.reserve(moves.size());
    for (const QVariant &moveVariant : moves) {
        const QVariantMap map = moveVariant.toMap();
        const QString componentId = map.value(QStringLiteral("id")).toString();
        if (componentId.isEmpty())
            continue;

        const qreal oldX = map.value(QStringLiteral("oldX")).toReal();
        const qreal oldY = map.value(QStringLiteral("oldY")).toReal();
        const qreal newX = map.value(QStringLiteral("newX")).toReal();
        const qreal newY = map.value(QStringLiteral("newY")).toReal();
        if (qFuzzyCompare(oldX, newX) && qFuzzyCompare(oldY, newY))
            continue;

        MoveComponentsCommand::MoveEntry entry;
        entry.componentId = componentId;
        entry.oldX = oldX;
        entry.oldY = oldY;
        entry.newX = newX;
        entry.newY = newY;
        entries.append(entry);
    }

    if (entries.isEmpty())
        return;

    if (entries.size() == 1) {
        const MoveComponentsCommand::MoveEntry &entry = entries.first();
        push(new MoveComponentCommand(graph,
                                      entry.componentId,
                                      entry.oldX,
                                      entry.oldY,
                                      entry.newX,
                                      entry.newY));
        return;
    }

    push(new MoveComponentsCommand(graph, entries));
}

void UndoStack::pushSetComponentGeometry(ComponentModel *component,
                                         qreal oldX,
                                         qreal oldY,
                                         qreal oldWidth,
                                         qreal oldHeight,
                                         qreal newX,
                                         qreal newY,
                                         qreal newWidth,
                                         qreal newHeight)
{
    if (!component)
        return;
    if (qFuzzyCompare(oldX, newX)
        && qFuzzyCompare(oldY, newY)
        && qFuzzyCompare(oldWidth, newWidth)
        && qFuzzyCompare(oldHeight, newHeight)) {
        return;
    }

    push(new SetComponentGeometryCommand(component,
                                         oldX,
                                         oldY,
                                         oldWidth,
                                         oldHeight,
                                         newX,
                                         newY,
                                         newWidth,
                                         newHeight));
}

void UndoStack::pushSetComponentProperty(ComponentModel *component,
                                         const QString &propertyName,
                                         const QVariant &newValue)
{
    if (!component || propertyName.trimmed().isEmpty())
        return;

    const QByteArray propertyUtf8 = propertyName.toUtf8();
    const QVariant oldValue = component->property(propertyUtf8.constData());
    if (oldValue == newValue)
        return;

    push(new SetComponentPropertyCommand(component, propertyUtf8, oldValue, newValue));
}

void UndoStack::pushSetConnectionProperty(ConnectionModel *connection,
                                          const QString &propertyName,
                                          const QVariant &newValue)
{
    if (!connection || !isConnectionPropertyUndoable(propertyName))
        return;

    const QByteArray propertyUtf8 = propertyName.toUtf8();
    const QVariant oldValue = connection->property(propertyUtf8.constData());
    if (oldValue == newValue)
        return;

    push(new SetConnectionPropertyCommand(connection, propertyUtf8, oldValue, newValue));
}

void UndoStack::pushAddConnection(GraphModel *graph, ConnectionModel *connection)
{
    if (!graph || !connection)
        return;

    push(new AddConnectionCommand(graph, connection));
}

void UndoStack::pushRemoveConnection(GraphModel *graph, const QString &connectionId)
{
    if (!graph || connectionId.isEmpty())
        return;

    push(new RemoveConnectionCommand(graph, connectionId));
}

void UndoStack::pushSetConnectionSides(ConnectionModel *connection,
                                      int sourceSide,
                                      int targetSide)
{
    if (!connection)
        return;

    const ConnectionModel::Side newSourceSide = sideFromInt(sourceSide);
    const ConnectionModel::Side newTargetSide = sideFromInt(targetSide);
    if (connection->sourceSide() == newSourceSide && connection->targetSide() == newTargetSide)
        return;

    push(new SetConnectionSidesCommand(connection,
                                       connection->sourceSide(),
                                       connection->targetSide(),
                                       newSourceSide,
                                       newTargetSide));
}

void UndoStack::undo()
{
    if (!canUndo()) return;

    const bool prevCanUndo = canUndo();
    const bool prevCanRedo = canRedo();
    const QString prevUndoText = undoText();
    const QString prevRedoText = redoText();
    const int prevCount = count();

    m_commands.at(m_index)->undo();
    --m_index;

    notifyChanges(prevCanUndo, prevCanRedo, prevUndoText, prevRedoText, prevCount);
}

void UndoStack::redo()
{
    if (!canRedo()) return;

    const bool prevCanUndo = canUndo();
    const bool prevCanRedo = canRedo();
    const QString prevUndoText = undoText();
    const QString prevRedoText = redoText();
    const int prevCount = count();

    ++m_index;
    m_commands.at(m_index)->redo();

    notifyChanges(prevCanUndo, prevCanRedo, prevUndoText, prevRedoText, prevCount);
}

void UndoStack::clear()
{
    if (m_commands.isEmpty()) return;

    const bool prevCanUndo = canUndo();
    const bool prevCanRedo = canRedo();
    const QString prevUndoText = undoText();
    const QString prevRedoText = redoText();
    const int prevCount = count();

    qDeleteAll(m_commands);
    m_commands.clear();
    m_index = -1;

    notifyChanges(prevCanUndo, prevCanRedo, prevUndoText, prevRedoText, prevCount);
}

void UndoStack::notifyChanges(bool prevCanUndo, bool prevCanRedo,
                               const QString &prevUndoText,
                               const QString &prevRedoText,
                               int prevCount)
{
    if (canUndo() != prevCanUndo) emit canUndoChanged();
    if (canRedo() != prevCanRedo) emit canRedoChanged();
    if (undoText() != prevUndoText) emit undoTextChanged();
    if (redoText() != prevRedoText) emit redoTextChanged();
    if (count() != prevCount) emit countChanged();
}

