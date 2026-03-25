#ifndef GRAPHEDITORCONTROLLER_H
#define GRAPHEDITORCONTROLLER_H

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include "models/GraphModel.h"
#include "commands/UndoStack.h"
#include "extensions/runtime/TypeRegistry.h"

/**
 * GraphEditorController orchestrates type-aware graph mutations.
 *
 * All mutating operations (createComponent, connectComponents) go through this
 * controller so that:
 *   1. Component defaults (width, height, color, shape) come from the
 *      TypeRegistry rather than from hardcoded constants.
 *   2. Connection policy is enforced before a command is pushed.
 *   3. Every accepted operation is pushed onto the UndoStack so
 *      undo/redo remains deterministic.
 *
 * GraphModel and UndoStack retain their full APIs for batch loading,
 * persistence, and regression tests — this controller is an additional
 * convenience layer, not a replacement.
 */
class GraphEditorController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel   *graph        READ graph        WRITE setGraph
                   NOTIFY graphChanged        FINAL)
    Q_PROPERTY(UndoStack    *undoStack    READ undoStack    WRITE setUndoStack
                   NOTIFY undoStackChanged    FINAL)
    Q_PROPERTY(TypeRegistry *typeRegistry READ typeRegistry WRITE setTypeRegistry
                   NOTIFY typeRegistryChanged FINAL)

public:
    explicit GraphEditorController(QObject *parent = nullptr);

    GraphModel   *graph()        const;
    UndoStack    *undoStack()    const;
    TypeRegistry *typeRegistry() const;

    void setGraph(GraphModel *graph);
    void setUndoStack(UndoStack *undoStack);
    void setTypeRegistry(TypeRegistry *registry);

    /**
    * Creates a new component of @p typeId at (@p x, @p y).
     * Default geometry and visual properties are read from the TypeRegistry
     * (falling back to built-in defaults when no registry is set or the type
     * is unknown).
     *
    * Returns the new component's ID on success, or an empty string if neither
     * graph nor undoStack is set.
     */
    Q_INVOKABLE QString createComponent(const QString &typeId, qreal x, qreal y);

    /**
     * ID-explicit variant — useful for scripted tests and replay scenarios
     * where a stable ID is required.
     */
    Q_INVOKABLE QString createComponentWithId(const QString &id,
                                              const QString &typeId,
                                              qreal x,
                                              qreal y);

    /**
    * Validates and creates a directed connection from the component
    * identified by @p sourceId to the component identified by @p targetId.
     *
     * Connection properties are normalized through the TypeRegistry before
     * the command is pushed.  Returns the new connection's ID on success,
     * or an empty string if the connection was rejected or required objects
     * are missing.
     *
     * On rejection, connectionRejected() is also emitted with the reason.
     */
    Q_INVOKABLE QString connectComponents(const QString &sourceId,
                                          const QString &targetId);

    /**
     * Returns the policy-rejection message from the most recent failed
    * connectComponents() call. Empty if the last call succeeded or no call was
     * made yet.
     */
    Q_INVOKABLE QString lastConnectionRejectionReason() const;

signals:
    void graphChanged();
    void undoStackChanged();
    void typeRegistryChanged();

    void componentCreated(const QString &componentId, const QString &typeId);

    /** Emitted after a successful connectComponents(). */
    void connectionCreated(const QString &connectionId,
                           const QString &sourceId,
                           const QString &targetId);

    /** Emitted when connectComponents() is blocked by a policy provider. */
    void connectionRejected(const QString &sourceId,
                            const QString &targetId,
                            const QString &reason);

private:
    // Resolves default component geometry/visuals for @p typeId.
    struct ComponentDefaults {
        qreal   width  = 120.0;
        qreal   height =  80.0;
        QString color  = QStringLiteral("#4fc3f7");
        QString shape  = QStringLiteral("rounded");
    };
    ComponentDefaults resolveComponentDefaults(const QString &typeId) const;

    GraphModel   *m_graph        = nullptr;
    UndoStack    *m_undoStack    = nullptr;
    TypeRegistry *m_typeRegistry = nullptr;
    QString       m_lastRejectionReason;
};

#endif // GRAPHEDITORCONTROLLER_H
