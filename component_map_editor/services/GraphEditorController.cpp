#include "GraphEditorController.h"

#include <QUuid>

#include "commands/GraphCommands.h"

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
    emit componentCreated(id, typeId);
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

QString GraphEditorController::lastConnectionRejectionReason() const
{
    return m_lastRejectionReason;
}
