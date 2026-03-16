#include "UndoStack.h"

#include "GraphCommands.h"

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
        delete command;
    } else {
        m_commands.append(command);
        ++m_index;
        command->redo();
    }

    notifyChanges(prevCanUndo, prevCanRedo, prevUndoText, prevRedoText, prevCount);
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

