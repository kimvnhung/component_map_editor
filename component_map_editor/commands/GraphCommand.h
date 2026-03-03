#ifndef GRAPHCOMMAND_H
#define GRAPHCOMMAND_H

#include <QString>

/**
 * GraphCommand is a lightweight abstract undo/redo command used by the
 * ComponentMapEditor module.  It intentionally avoids a dependency on Qt Widgets
 * (QUndoCommand) so that the module can be used in QML-only applications.
 */
class GraphCommand
{
public:
    explicit GraphCommand(const QString &text = QString()) : m_text(text) {}
    virtual ~GraphCommand() = default;

    virtual void redo() = 0;
    virtual void undo() = 0;

    virtual QString text() const { return m_text; }

    // Return a non-negative id to enable merging of successive commands of the
    // same type (e.g. continuous node-drag moves).  Default -1 = no merging.
    virtual int id() const { return -1; }

    // Called on the *older* command when a newer command with the same id() is
    // pushed.  Returns true when merged (the newer command is discarded).
    virtual bool mergeWith(const GraphCommand * /*newer*/) { return false; }

private:
    QString m_text;
};

#endif // GRAPHCOMMAND_H
