#ifndef GRAPHEDITORCONTROLLER_H
#define GRAPHEDITORCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QQmlEngine>

#include "models/GraphModel.h"
#include "commands/UndoStack.h"
#include "extensions/runtime/TypeRegistry.h"
#include "services/InvariantChecker.h"

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

    Q_PROPERTY(GraphModel      *graph            READ graph            WRITE setGraph
                   NOTIFY graphChanged            FINAL)
    Q_PROPERTY(UndoStack       *undoStack        READ undoStack        WRITE setUndoStack
                   NOTIFY undoStackChanged        FINAL)
    Q_PROPERTY(TypeRegistry    *typeRegistry     READ typeRegistry     WRITE setTypeRegistry
                   NOTIFY typeRegistryChanged     FINAL)
    Q_PROPERTY(InvariantChecker *invariantChecker READ invariantChecker WRITE setInvariantChecker
                   NOTIFY invariantCheckerChanged FINAL)
    Q_PROPERTY(bool             strictMode       READ strictMode       WRITE setStrictMode
                   NOTIFY strictModeChanged       FINAL)

public:
    explicit GraphEditorController(QObject *parent = nullptr);

    GraphModel      *graph()            const;
    UndoStack       *undoStack()        const;
    TypeRegistry    *typeRegistry()     const;
    InvariantChecker *invariantChecker() const;
    bool             strictMode()       const;

    void setGraph(GraphModel *graph);
    void setUndoStack(UndoStack *undoStack);
    void setTypeRegistry(TypeRegistry *registry);
    void setInvariantChecker(InvariantChecker *checker);
    void setStrictMode(bool strict);

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

    // Creates a component for palette/menu flows with explicit visual fields.
    // Width/height <= 0 fallback to type defaults.
    Q_INVOKABLE QString createPaletteComponent(const QString &typeId,
                                               const QString &title,
                                               const QString &icon,
                                               const QString &color,
                                               qreal x,
                                               qreal y,
                                               qreal width,
                                               qreal height);

    // Duplicates an existing component with positional offset.
    Q_INVOKABLE QString duplicateComponent(const QString &sourceId,
                                           qreal offsetX = 40.0,
                                           qreal offsetY = 40.0);

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

    // Deterministic connect API for drag-drop flows.
    // preferredConnectionId defaults to "conn_<source>_<target>" if empty.
    // Returns empty if invalid, rejected by policy, or duplicate id already exists.
    Q_INVOKABLE QString connectComponentsFromDrag(const QString &sourceId,
                                                  const QString &targetId,
                                                  int sourceSide,
                                                  int targetSide = -1,
                                                  const QString &preferredConnectionId = QString(),
                                                  const QString &fallbackLabel = QStringLiteral("path A"));

    // Commits grouped move operations through UndoStack command batching.
    Q_INVOKABLE bool commitMoveBatch(const QVariantList &moves);

    // Commits resize geometry through UndoStack to preserve undo semantics.
    Q_INVOKABLE bool commitResize(const QString &componentId,
                                  qreal oldX,
                                  qreal oldY,
                                  qreal oldWidth,
                                  qreal oldHeight,
                                  qreal newX,
                                  qreal newY,
                                  qreal newWidth,
                                  qreal newHeight);

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
    void invariantCheckerChanged();
    void strictModeChanged();

    void componentCreated(const QString &componentId, const QString &typeId);

    /** Emitted after a successful connectComponents(). */
    void connectionCreated(const QString &connectionId,
                           const QString &sourceId,
                           const QString &targetId);

    /** Emitted when connectComponents() is blocked by a policy provider. */
    void connectionRejected(const QString &sourceId,
                            const QString &targetId,
                            const QString &reason);

    // ── Strict-mode observability signals ─────────────────────────────────
    // Emitted when a UI mutation is blocked by a pre-check invariant violation
    // or when a post-check violation fires (before rollback).
    void mutationBlocked(const QString &operationType, const QString &reason);

    // Emitted after a post-mutation invariant failure.  The command is rolled
    // back before this signal fires.
    void invariantViolationRolledBack(const QString &operationType,
                                      const QString &violation);

private:
    // Resolves default component geometry/visuals for @p typeId.
    struct ComponentDefaults {
        qreal   width  = 120.0;
        qreal   height =  80.0;
        QString color  = QStringLiteral("#4fc3f7");
        QString shape  = QStringLiteral("rounded");
    };
    ComponentDefaults resolveComponentDefaults(const QString &typeId) const;
    QVariantMap buildConnectionPolicyContext(const QString &sourceId,
                                             const QString &targetId) const;

    GraphModel   *m_graph        = nullptr;
    UndoStack    *m_undoStack    = nullptr;
    TypeRegistry *m_typeRegistry = nullptr;
    QString       m_lastRejectionReason;

    // ── Strict-mode enforcement ───────────────────────────────────────────
    // When strictMode is true, every write method runs pre- and post-mutation
    // invariant checks via m_invariantChecker.  A pre-failure rejects the
    // mutation before any command is pushed.  A post-failure rolls back the
    // last-pushed command via UndoStack::undo() + discardRedoHistory().
    InvariantChecker *m_invariantChecker = nullptr;
    bool              m_strictMode       = false;

    // Returns true (pass) or false (blocked); emits mutationBlocked on false.
    bool preCheckInvariants(const QString &operationType, QString *error);

    // Returns true (pass) or false (rolled back); emits
    // invariantViolationRolledBack + mutationBlocked on false.
    bool postCheckAndRollback(const QString &operationType, QString *error);
};

#endif // GRAPHEDITORCONTROLLER_H
