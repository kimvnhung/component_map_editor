#include "GraphModel.h"

#include <QtAlgorithms>

GraphModel::GraphModel(QObject *parent)
    : QObject(parent)
{}

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
    // In batch mode skip the O(n) duplicate check; caller guarantees unique IDs.
    if (!m_batchMode && componentById(component->id()))
        return;
    component->setParent(this);
    m_components.append(component);
    emit componentAdded(component);
    if (!m_batchMode)
        emit componentsChanged();
}

bool GraphModel::removeComponent(const QString &id)
{
    for (int i = 0; i < m_components.size(); ++i) {
        if (m_components.at(i)->id() == id) {
            ComponentModel *component = m_components.takeAt(i);
            emit componentRemoved(id);
            emit componentsChanged();
            component->setParent(nullptr);
            return true;
        }
    }
    return false;
}

ComponentModel *GraphModel::componentById(const QString &id) const
{
    for (ComponentModel *component : m_components) {
        if (component->id() == id)
            return component;
    }
    return nullptr;
}

void GraphModel::addConnection(ConnectionModel *connection)
{
    if (!connection)
        return;
    // In batch mode skip the O(n) duplicate check; caller guarantees unique IDs.
    if (!m_batchMode && connectionById(connection->id()))
        return;
    connection->setParent(this);
    m_connections.append(connection);
    emit connectionAdded(connection);
    if (!m_batchMode)
        emit connectionsChanged();
}

bool GraphModel::removeConnection(const QString &id)
{
    for (int i = 0; i < m_connections.size(); ++i) {
        if (m_connections.at(i)->id() == id) {
            ConnectionModel *connection = m_connections.takeAt(i);
            emit connectionRemoved(id);
            emit connectionsChanged();
            connection->setParent(nullptr);
            return true;
        }
    }
    return false;
}

ConnectionModel *GraphModel::connectionById(const QString &id) const
{
    for (ConnectionModel *connection : m_connections) {
        if (connection->id() == id)
            return connection;
    }
    return nullptr;
}

void GraphModel::clear()
{
    if (!m_connections.isEmpty()) {
        qDeleteAll(m_connections);
        m_connections.clear();
        if (!m_batchMode)
            emit connectionsChanged();
    }
    if (!m_components.isEmpty()) {
        qDeleteAll(m_components);
        m_components.clear();
        if (!m_batchMode)
            emit componentsChanged();
    }
}

void GraphModel::beginBatchUpdate()
{
    m_batchMode = true;
}

void GraphModel::endBatchUpdate()
{
    m_batchMode = false;
    emit componentsChanged();
    emit connectionsChanged();
}
