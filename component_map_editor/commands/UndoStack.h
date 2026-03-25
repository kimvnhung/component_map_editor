#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QObject>
#include <QList>
#include <QQmlEngine>

#include "GraphCommand.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"

/**
 * UndoStack manages a stack of GraphCommand objects and exposes undo/redo
 * functionality to QML.  It does not depend on Qt Widgets.
 */
class UndoStack : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged FINAL)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged FINAL)
    Q_PROPERTY(QString undoText READ undoText NOTIFY undoTextChanged FINAL)
    Q_PROPERTY(QString redoText READ redoText NOTIFY redoTextChanged FINAL)
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)

public:
    explicit UndoStack(QObject *parent = nullptr);
    ~UndoStack() override;

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;
    int count() const;

    // Takes ownership of the command and executes redo() immediately.
    void push(GraphCommand *command);

    Q_INVOKABLE void pushAddComponent(GraphModel *graph, ComponentModel *component);
    Q_INVOKABLE void pushRemoveComponent(GraphModel *graph, const QString &componentId);
    Q_INVOKABLE void pushMoveComponent(GraphModel *graph,
                                       const QString &componentId,
                                       qreal oldX,
                                       qreal oldY,
                                       qreal newX,
                                       qreal newY);
    Q_INVOKABLE void pushMoveComponents(GraphModel *graph, const QVariantList &moves);
    Q_INVOKABLE void pushSetComponentGeometry(ComponentModel *component,
                                              qreal oldX,
                                              qreal oldY,
                                              qreal oldWidth,
                                              qreal oldHeight,
                                              qreal newX,
                                              qreal newY,
                                              qreal newWidth,
                                              qreal newHeight);
    Q_INVOKABLE void pushSetComponentProperty(ComponentModel *component,
                                              const QString &propertyName,
                                              const QVariant &newValue);
    Q_INVOKABLE void pushSetConnectionProperty(ConnectionModel *connection,
                                               const QString &propertyName,
                                               const QVariant &newValue);

    Q_INVOKABLE void pushAddConnection(GraphModel *graph, ConnectionModel *connection);
    Q_INVOKABLE void pushRemoveConnection(GraphModel *graph, const QString &connectionId);
    Q_INVOKABLE void pushSetConnectionSides(ConnectionModel *connection,
                                            int sourceSide,
                                            int targetSide);

public slots:
    void undo();
    void redo();
    void clear();
    void discardRedoHistory();

signals:
    void canUndoChanged();
    void canRedoChanged();
    void undoTextChanged();
    void redoTextChanged();
    void countChanged();

private:
    void notifyChanges(bool prevCanUndo, bool prevCanRedo,
                       const QString &prevUndoText, const QString &prevRedoText,
                       int prevCount);

    QList<GraphCommand *> m_commands;
    int m_index = -1; // index of the last executed command (-1 = empty)
};

#endif // UNDOSTACK_H

