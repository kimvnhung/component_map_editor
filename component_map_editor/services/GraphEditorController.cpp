#include "GraphEditorController.h"

#include <QUuid>

#include "commands/GraphCommands.h"
#include "InvariantChecker.h"

GraphEditorController::GraphEditorController(QObject *parent)
    : QObject(parent)
{}

// ---------------------------------------------------------------------------
// Property accessors
// ---------------------------------------------------------------------------

GraphModel *GraphEditorController::graph() const        { return m_graph; }
UndoStack  *GraphEditorController::undoStack() const    { return m_undoStack; }
TypeRegistry *GraphEditorController::typeRegistry() const { return m_typeRegistry; }

void GraphEditorController::setGraph(GraphModel *graph)
{
    if (m_graph == graph)
        return;
    m_graph = graph;
    emit graphChanged();
}

void GraphEditorController::setUndoStack(UndoStack *undoStack)
{
    if (m_undoStack == undoStack)
        return;
    m_undoStack = undoStack;
    emit undoStackChanged();
}

void GraphEditorController::setTypeRegistry(TypeRegistry *registry)
{
    if (m_typeRegistry == registry)
        return;
    m_typeRegistry = registry;
    emit typeRegistryChanged();
}

InvariantChecker *GraphEditorController::invariantChecker() const { return m_invariantChecker; }
bool              GraphEditorController::strictMode()        const { return m_strictMode; }

void GraphEditorController::setInvariantChecker(InvariantChecker *checker)
{
    if (m_invariantChecker == checker)
        return;
    m_invariantChecker = checker;
    emit invariantCheckerChanged();
}

void GraphEditorController::setStrictMode(bool strict)
{
    if (m_strictMode == strict)
        return;
    m_strictMode = strict;
    emit strictModeChanged();
}

// ---------------------------------------------------------------------------
// Strict-mode invariant helpers
// ---------------------------------------------------------------------------

// Runs pre-mutation invariant check when strictMode is on.
// Returns true (pass through) or false (mutation blocked).
// Emits mutationBlocked on failure.
bool GraphEditorController::preCheckInvariants(const QString &operationType, QString *error)
{
    if (!m_strictMode || !m_invariantChecker || !m_graph)
        return true;

    QString violation;
    if (!m_invariantChecker->checkAll(m_graph, &violation)) {
        const QString msg =
            QStringLiteral("Pre-mutation invariant violation [%1]: %2")
                .arg(operationType, violation);
        if (error)
            *error = msg;
        emit mutationBlocked(operationType, msg);
        return false;
    }
    return true;
}

// Runs post-mutation invariant check when strictMode is on.
// On failure: rolls back the last-pushed command, emits signals, returns false.
bool GraphEditorController::postCheckAndRollback(const QString &operationType, QString *error)
{
    if (!m_strictMode || !m_invariantChecker || !m_graph)
        return true;

    QString violation;
    if (!m_invariantChecker->checkAll(m_graph, &violation)) {
        if (m_undoStack && m_undoStack->canUndo())
            m_undoStack->undo();
        if (m_undoStack)
            m_undoStack->discardRedoHistory();

        if (error)
            *error = violation;
        emit invariantViolationRolledBack(operationType, violation);
        emit mutationBlocked(operationType, violation);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

GraphEditorController::ComponentDefaults
GraphEditorController::resolveComponentDefaults(const QString &typeId) const
{
    ComponentDefaults d; // built-in fallback values

    if (!m_typeRegistry || typeId.isEmpty() || !m_typeRegistry->hasComponentType(typeId))
        return d;

    const QVariantMap desc = m_typeRegistry->componentTypeDescriptor(typeId);
    if (desc.contains(QStringLiteral("defaultWidth")))
        d.width  = desc.value(QStringLiteral("defaultWidth")).toReal();
    if (desc.contains(QStringLiteral("defaultHeight")))
        d.height = desc.value(QStringLiteral("defaultHeight")).toReal();
    if (desc.contains(QStringLiteral("defaultColor")))
        d.color  = desc.value(QStringLiteral("defaultColor")).toString();
    if (desc.contains(QStringLiteral("shape")))
        d.shape  = desc.value(QStringLiteral("shape")).toString();

    return d;
}

// ---------------------------------------------------------------------------
// createComponent
// ---------------------------------------------------------------------------

QString GraphEditorController::createComponent(const QString &typeId, qreal x, qreal y)
{
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    return createComponentWithId(id, typeId, x, y);
}

QString GraphEditorController::createComponentWithId(const QString &id,
                                                     const QString &typeId,
                                                     qreal x,
                                                     qreal y)
{
    if (!m_graph || !m_undoStack || id.isEmpty())
        return {};

    {
        QString err;
        if (!preCheckInvariants(QStringLiteral("createComponentWithId"), &err))
            return {};
    }

    const ComponentDefaults d = resolveComponentDefaults(typeId);

    auto *component = new ComponentModel(id, typeId.isEmpty()
                                             ? QStringLiteral("New Component")
                                             : typeId,
                                         x, y, d.color, typeId);
    component->setWidth(d.width);
    component->setHeight(d.height);
    component->setShape(d.shape);

    // Apply type-specific extra default properties (e.g. priority, condition).
    if (m_typeRegistry && m_typeRegistry->hasComponentType(typeId)) {
        const QVariantMap extras = m_typeRegistry->defaultComponentProperties(typeId);
        for (auto it = extras.constBegin(); it != extras.constEnd(); ++it)
            component->setProperty(it.key().toUtf8().constData(), it.value());
    }

    m_undoStack->pushAddComponent(m_graph, component);

    {
        QString err;
        if (!postCheckAndRollback(QStringLiteral("createComponentWithId"), &err))
            return {};
    }

    emit componentCreated(id, typeId);
    return id;
}

QString GraphEditorController::createPaletteComponent(const QString &typeId,
                                                      const QString &title,
                                                      const QString &icon,
                                                      const QString &color,
                                                      qreal x,
                                                      qreal y,
                                                      qreal width,
                                                      qreal height)
{
    if (!m_graph || !m_undoStack)
        return {};

    {
        QString err;
        if (!preCheckInvariants(QStringLiteral("createPaletteComponent"), &err))
            return {};
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const ComponentDefaults d = resolveComponentDefaults(typeId);

    auto *component = new ComponentModel(id,
                                         title.isEmpty() ? (typeId.isEmpty() ? QStringLiteral("Component") : typeId)
                                                         : title,
                                         x,
                                         y,
                                         color.isEmpty() ? d.color : color,
                                         typeId,
                                         m_graph);
    component->setIcon(icon.isEmpty() ? QStringLiteral("cube") : icon);
    component->setWidth(width > 0.0 ? width : d.width);
    component->setHeight(height > 0.0 ? height : d.height);
    component->setShape(d.shape);

    if (m_typeRegistry && m_typeRegistry->hasComponentType(typeId)) {
        const QVariantMap extras = m_typeRegistry->defaultComponentProperties(typeId);
        for (auto it = extras.constBegin(); it != extras.constEnd(); ++it)
            component->setProperty(it.key().toUtf8().constData(), it.value());
    }

    m_undoStack->pushAddComponent(m_graph, component);

    {
        QString err;
        if (!postCheckAndRollback(QStringLiteral("createPaletteComponent"), &err))
            return {};
    }

    emit componentCreated(id, typeId);
    return id;
}

QString GraphEditorController::duplicateComponent(const QString &sourceId,
                                                  qreal offsetX,
                                                  qreal offsetY)
{
    if (!m_graph || !m_undoStack || sourceId.isEmpty())
        return {};

    {
        QString err;
        if (!preCheckInvariants(QStringLiteral("duplicateComponent"), &err))
            return {};
    }

    ComponentModel *source = m_graph->componentById(sourceId);
    if (!source)
        return {};

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto *copy = new ComponentModel(id,
                                    source->title() + QStringLiteral(" Copy"),
                                    source->x() + offsetX,
                                    source->y() + offsetY,
                                    source->color(),
                                    source->type(),
                                    m_graph);
    copy->setContent(source->content());
    copy->setIcon(source->icon());
    copy->setWidth(source->width());
    copy->setHeight(source->height());
    copy->setShape(source->shape());

    m_undoStack->pushAddComponent(m_graph, copy);

    {
        QString err;
        if (!postCheckAndRollback(QStringLiteral("duplicateComponent"), &err))
            return {};
    }

    emit componentCreated(id, copy->type());
    return id;
}

// ---------------------------------------------------------------------------
// connectComponents
// ---------------------------------------------------------------------------

QString GraphEditorController::connectComponents(const QString &sourceId,
                                                 const QString &targetId)
{
    if (!m_graph || !m_undoStack)
        return {};

    ComponentModel *src = m_graph->componentById(sourceId);
    ComponentModel *tgt = m_graph->componentById(targetId);
    if (!src || !tgt)
        return {};

    const QString srcType = src->type();
    const QString tgtType = tgt->type();

    // Connection-policy gate
    if (m_typeRegistry) {
        QString reason;
        if (!m_typeRegistry->canConnect(srcType, tgtType, {}, &reason)) {
            m_lastRejectionReason = reason;
            emit connectionRejected(sourceId, targetId, reason);
            return {};
        }
    }

    m_lastRejectionReason.clear();

    // Normalize properties through policy providers
    QVariantMap props;
    if (m_typeRegistry)
        props = m_typeRegistry->normalizeConnectionProperties(srcType, tgtType, {});

    const QString connId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QString label  = props.value(QStringLiteral("label")).toString();

    auto *connection = new ConnectionModel(connId, sourceId, targetId, label);

    m_undoStack->pushAddConnection(m_graph, connection);
    emit connectionCreated(connId, sourceId, targetId);
    return connId;
}

QString GraphEditorController::connectComponentsFromDrag(const QString &sourceId,
                                                         const QString &targetId,
                                                         int sourceSide,
                                                         int targetSide,
                                                         const QString &preferredConnectionId,
                                                         const QString &fallbackLabel)
{
    if (!m_graph || !m_undoStack)
        return {};
    if (sourceId.isEmpty() || targetId.isEmpty() || sourceId == targetId)
        return {};

    {
        QString err;
        if (!preCheckInvariants(QStringLiteral("connectComponentsFromDrag"), &err))
            return {};
    }

    ComponentModel *src = m_graph->componentById(sourceId);
    ComponentModel *tgt = m_graph->componentById(targetId);
    if (!src || !tgt)
        return {};

    const QString srcType = src->type();
    const QString tgtType = tgt->type();
    if (m_typeRegistry) {
        QString reason;
        if (!m_typeRegistry->canConnect(srcType, tgtType, {}, &reason)) {
            m_lastRejectionReason = reason;
            emit connectionRejected(sourceId, targetId, reason);
            return {};
        }
    }

    m_lastRejectionReason.clear();

    QString connId = preferredConnectionId;
    if (connId.isEmpty())
        connId = QStringLiteral("conn_%1_%2").arg(sourceId, targetId);

    if (m_graph->connectionById(connId))
        return {};

    QVariantMap props;
    if (m_typeRegistry)
        props = m_typeRegistry->normalizeConnectionProperties(srcType, tgtType, {});

    const QString label = props.value(QStringLiteral("label")).toString().isEmpty()
        ? fallbackLabel
        : props.value(QStringLiteral("label")).toString();

    m_undoStack->pushAddConnectionBySpec(m_graph,
                                         connId,
                                         sourceId,
                                         targetId,
                                         label,
                                         sourceSide,
                                         targetSide);

    {
        QString err;
        if (!postCheckAndRollback(QStringLiteral("connectComponentsFromDrag"), &err))
            return {};
    }

    emit connectionCreated(connId, sourceId, targetId);
    return connId;
}

bool GraphEditorController::commitMoveBatch(const QVariantList &moves)
{
    if (!m_graph || !m_undoStack || moves.isEmpty())
        return false;

    {
        QString err;
        if (!preCheckInvariants(QStringLiteral("commitMoveBatch"), &err))
            return false;
    }

    const int before = m_undoStack->count();
    m_undoStack->pushMoveComponents(m_graph, moves);

    {
        QString err;
        if (!postCheckAndRollback(QStringLiteral("commitMoveBatch"), &err))
            return false;
    }

    return m_undoStack->count() > before;
}

bool GraphEditorController::commitResize(const QString &componentId,
                                         qreal oldX,
                                         qreal oldY,
                                         qreal oldWidth,
                                         qreal oldHeight,
                                         qreal newX,
                                         qreal newY,
                                         qreal newWidth,
                                         qreal newHeight)
{
    if (!m_undoStack || componentId.isEmpty())
        return false;

    ComponentModel *component = m_graph ? m_graph->componentById(componentId) : nullptr;
    if (!component)
        return false;

    {
        QString err;
        if (!preCheckInvariants(QStringLiteral("commitResize"), &err))
            return false;
    }

    const int before = m_undoStack->count();
    m_undoStack->pushSetComponentGeometry(component,
                                          oldX,
                                          oldY,
                                          oldWidth,
                                          oldHeight,
                                          newX,
                                          newY,
                                          newWidth,
                                          newHeight);

    {
        QString err;
        if (!postCheckAndRollback(QStringLiteral("commitResize"), &err))
            return false;
    }

    return m_undoStack->count() > before;
}

QString GraphEditorController::lastConnectionRejectionReason() const
{
    return m_lastRejectionReason;
}
