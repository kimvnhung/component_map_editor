#include "GraphModel.h"

#include <QtAlgorithms>

GraphModel::GraphModel(QObject *parent)
    : QObject(parent)
{}

ComponentModel *GraphModel::firstComponentByIdLinear(const QString &id) const
{
    for (ComponentModel *component : m_components) {
        if (component && component->id() == id)
            return component;
    }
    return nullptr;
}

ConnectionModel *GraphModel::firstConnectionByIdLinear(const QString &id) const
{
    for (ConnectionModel *connection : m_connections) {
        if (connection && connection->id() == id)
            return connection;
    }
    return nullptr;
}

void GraphModel::attachComponent(ComponentModel *component)
{
    if (!component)
        return;

    QObject::disconnect(component, &ComponentModel::idChanged, this, nullptr);

    m_componentTrackedIds.insert(component, component->id());
    if (!m_componentIndex.contains(component->id()))
        m_componentIndex.insert(component->id(), component);

    QObject::connect(component, &ComponentModel::idChanged, this, [this, component]() {
        onTrackedComponentIdChanged(component);
    });
}

void GraphModel::detachComponent(ComponentModel *component)
{
    if (!component)
        return;

    QObject::disconnect(component, nullptr, this, nullptr);

    const QString oldId = m_componentTrackedIds.take(component);
    if (!oldId.isNull() && m_componentIndex.value(oldId) == component) {
        ComponentModel *replacement = firstComponentByIdLinear(oldId);
        if (replacement)
            m_componentIndex.insert(oldId, replacement);
        else
            m_componentIndex.remove(oldId);
    }
}

void GraphModel::attachConnection(ConnectionModel *connection)
{
    if (!connection)
        return;

    QObject::disconnect(connection, &ConnectionModel::idChanged, this, nullptr);

    m_connectionTrackedIds.insert(connection, connection->id());
    if (!m_connectionIndex.contains(connection->id()))
        m_connectionIndex.insert(connection->id(), connection);

    QObject::connect(connection, &ConnectionModel::idChanged, this, [this, connection]() {
        onTrackedConnectionIdChanged(connection);
    });
}

void GraphModel::detachConnection(ConnectionModel *connection)
{
    if (!connection)
        return;

    QObject::disconnect(connection, nullptr, this, nullptr);

    const QString oldId = m_connectionTrackedIds.take(connection);
    if (!oldId.isNull() && m_connectionIndex.value(oldId) == connection) {
        ConnectionModel *replacement = firstConnectionByIdLinear(oldId);
        if (replacement)
            m_connectionIndex.insert(oldId, replacement);
        else
            m_connectionIndex.remove(oldId);
    }
}

void GraphModel::onTrackedComponentIdChanged(ComponentModel *component)
{
    if (!component)
        return;

    const QString oldId = m_componentTrackedIds.value(component);
    const QString newId = component->id();
    if (oldId == newId)
        return;

    m_componentTrackedIds[component] = newId;

    if (m_componentIndex.value(oldId) == component) {
        ComponentModel *replacement = firstComponentByIdLinear(oldId);
        if (replacement)
            m_componentIndex.insert(oldId, replacement);
        else
            m_componentIndex.remove(oldId);
    }

    ComponentModel *currentFirst = m_componentIndex.value(newId, nullptr);
    if (!currentFirst) {
        m_componentIndex.insert(newId, component);
    } else if (currentFirst != component) {
        const int changedPos = m_components.indexOf(component);
        const int currentPos = m_components.indexOf(currentFirst);
        if (changedPos != -1 && (currentPos == -1 || changedPos < currentPos))
            m_componentIndex.insert(newId, component);
    }

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif
}

void GraphModel::onTrackedConnectionIdChanged(ConnectionModel *connection)
{
    if (!connection)
        return;

    const QString oldId = m_connectionTrackedIds.value(connection);
    const QString newId = connection->id();
    if (oldId == newId)
        return;

    m_connectionTrackedIds[connection] = newId;

    if (m_connectionIndex.value(oldId) == connection) {
        ConnectionModel *replacement = firstConnectionByIdLinear(oldId);
        if (replacement)
            m_connectionIndex.insert(oldId, replacement);
        else
            m_connectionIndex.remove(oldId);
    }

    ConnectionModel *currentFirst = m_connectionIndex.value(newId, nullptr);
    if (!currentFirst) {
        m_connectionIndex.insert(newId, connection);
    } else if (currentFirst != connection) {
        const int changedPos = m_connections.indexOf(connection);
        const int currentPos = m_connections.indexOf(currentFirst);
        if (changedPos != -1 && (currentPos == -1 || changedPos < currentPos))
            m_connectionIndex.insert(newId, connection);
    }

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif
}

#ifdef QT_DEBUG
bool GraphModel::verifyComponentIndexIntegrity() const
{
    if (m_componentTrackedIds.size() != m_components.size())
        return false;

    for (ComponentModel *component : m_components) {
        if (!component)
            return false;
        if (!m_componentTrackedIds.contains(component))
            return false;
        if (m_componentTrackedIds.value(component) != component->id())
            return false;
    }

    for (auto it = m_componentIndex.constBegin(); it != m_componentIndex.constEnd(); ++it) {
        if (it.value() != firstComponentByIdLinear(it.key()))
            return false;
    }

    for (ComponentModel *component : m_components) {
        ComponentModel *expected = firstComponentByIdLinear(component->id());
        if (expected && m_componentIndex.value(component->id()) != expected)
            return false;
    }

    return true;
}

bool GraphModel::verifyConnectionIndexIntegrity() const
{
    if (m_connectionTrackedIds.size() != m_connections.size())
        return false;

    for (ConnectionModel *connection : m_connections) {
        if (!connection)
            return false;
        if (!m_connectionTrackedIds.contains(connection))
            return false;
        if (m_connectionTrackedIds.value(connection) != connection->id())
            return false;
    }

    for (auto it = m_connectionIndex.constBegin(); it != m_connectionIndex.constEnd(); ++it) {
        if (it.value() != firstConnectionByIdLinear(it.key()))
            return false;
    }

    for (ConnectionModel *connection : m_connections) {
        ConnectionModel *expected = firstConnectionByIdLinear(connection->id());
        if (expected && m_connectionIndex.value(connection->id()) != expected)
            return false;
    }

    return true;
}

void GraphModel::assertIndexIntegrity() const
{
    const bool ok = verifyComponentIndexIntegrity() && verifyConnectionIndexIntegrity();
    Q_ASSERT_X(ok, "GraphModel::assertIndexIntegrity",
               "GraphModel internal id index diverged from source collections");
}
#endif

QVariantList GraphModel::componentsVariant() const
{
    QVariantList list;
    list.reserve(m_components.size());
    for (ComponentModel *component : m_components)
        list.append(QVariant::fromValue(component));
    return list;
}

QVariantList GraphModel::connectionsVariant() const
{
    QVariantList list;
    list.reserve(m_connections.size());
    for (ConnectionModel *connection : m_connections)
        list.append(QVariant::fromValue(connection));
    return list;
}

int GraphModel::componentCount() const { return m_components.size(); }
int GraphModel::connectionCount() const { return m_connections.size(); }

const QList<ComponentModel *> &GraphModel::componentList() const { return m_components; }
const QList<ConnectionModel *> &GraphModel::connectionList() const { return m_connections; }

void GraphModel::addComponent(ComponentModel *component)
{
    if (!component)
        return;
    // In batch mode skip duplicate check; caller guarantees unique IDs.
    if (!m_batchMode && m_componentIndex.contains(component->id()))
        return;

    component->setParent(this);
    m_components.append(component);
    attachComponent(component);
    emit componentAdded(component);
    if (!m_batchMode)
        emit componentsChanged();

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif
}

bool GraphModel::removeComponent(const QString &id)
{
    ComponentModel *component = m_componentIndex.value(id, nullptr);
    if (!component)
        return false;

    const int index = m_components.indexOf(component);
    if (index < 0)
        return false;

    m_components.takeAt(index);
    detachComponent(component);
    emit componentRemoved(id);
    emit componentsChanged();
    component->setParent(nullptr);

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif

    return true;
}

ComponentModel *GraphModel::componentById(const QString &id) const
{
    return m_componentIndex.value(id, nullptr);
}

void GraphModel::addConnection(ConnectionModel *connection)
{
    if (!connection)
        return;
    // In batch mode skip duplicate check; caller guarantees unique IDs.
    if (!m_batchMode && m_connectionIndex.contains(connection->id()))
        return;

    connection->setParent(this);
    m_connections.append(connection);
    attachConnection(connection);
    emit connectionAdded(connection);
    if (!m_batchMode)
        emit connectionsChanged();

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif
}

bool GraphModel::removeConnection(const QString &id)
{
    ConnectionModel *connection = m_connectionIndex.value(id, nullptr);
    if (!connection)
        return false;

    const int index = m_connections.indexOf(connection);
    if (index < 0)
        return false;

    m_connections.takeAt(index);
    detachConnection(connection);
    emit connectionRemoved(id);
    emit connectionsChanged();
    connection->setParent(nullptr);

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif

    return true;
}

ConnectionModel *GraphModel::connectionById(const QString &id) const
{
    return m_connectionIndex.value(id, nullptr);
}

void GraphModel::clear()
{
    if (!m_connections.isEmpty()) {
        for (ConnectionModel *connection : std::as_const(m_connections))
            detachConnection(connection);
        qDeleteAll(m_connections);
        m_connections.clear();
        m_connectionIndex.clear();
        m_connectionTrackedIds.clear();
        if (!m_batchMode)
            emit connectionsChanged();
    }
    if (!m_components.isEmpty()) {
        for (ComponentModel *component : std::as_const(m_components))
            detachComponent(component);
        qDeleteAll(m_components);
        m_components.clear();
        m_componentIndex.clear();
        m_componentTrackedIds.clear();
        if (!m_batchMode)
            emit componentsChanged();
    }

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif
}

void GraphModel::beginBatchUpdate()
{
    m_batchMode = true;

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif
}

void GraphModel::endBatchUpdate()
{
    m_batchMode = false;

#ifdef QT_DEBUG
    assertIndexIntegrity();
#endif

    emit componentsChanged();
    emit connectionsChanged();
}
