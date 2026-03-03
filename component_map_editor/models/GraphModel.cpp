#include "GraphModel.h"

GraphModel::GraphModel(QObject *parent)
    : QObject(parent)
{}

QVariantList GraphModel::nodesVariant() const
{
    QVariantList list;
    list.reserve(m_nodes.size());
    for (NodeModel *n : m_nodes)
        list.append(QVariant::fromValue(n));
    return list;
}

QVariantList GraphModel::edgesVariant() const
{
    QVariantList list;
    list.reserve(m_edges.size());
    for (EdgeModel *e : m_edges)
        list.append(QVariant::fromValue(e));
    return list;
}

int GraphModel::nodeCount() const { return m_nodes.size(); }
int GraphModel::edgeCount() const { return m_edges.size(); }

const QList<NodeModel *> &GraphModel::nodeList() const { return m_nodes; }
const QList<EdgeModel *> &GraphModel::edgeList() const { return m_edges; }

void GraphModel::addNode(NodeModel *node)
{
    if (!node || nodeById(node->id()))
        return;
    node->setParent(this);
    m_nodes.append(node);
    emit nodeAdded(node);
    emit nodesChanged();
}

bool GraphModel::removeNode(const QString &id)
{
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes.at(i)->id() == id) {
            NodeModel *node = m_nodes.takeAt(i);
            emit nodeRemoved(id);
            emit nodesChanged();
            node->setParent(nullptr);
            return true;
        }
    }
    return false;
}

NodeModel *GraphModel::nodeById(const QString &id) const
{
    for (NodeModel *n : m_nodes) {
        if (n->id() == id)
            return n;
    }
    return nullptr;
}

void GraphModel::addEdge(EdgeModel *edge)
{
    if (!edge || edgeById(edge->id()))
        return;
    edge->setParent(this);
    m_edges.append(edge);
    emit edgeAdded(edge);
    emit edgesChanged();
}

bool GraphModel::removeEdge(const QString &id)
{
    for (int i = 0; i < m_edges.size(); ++i) {
        if (m_edges.at(i)->id() == id) {
            EdgeModel *edge = m_edges.takeAt(i);
            emit edgeRemoved(id);
            emit edgesChanged();
            edge->setParent(nullptr);
            return true;
        }
    }
    return false;
}

EdgeModel *GraphModel::edgeById(const QString &id) const
{
    for (EdgeModel *e : m_edges) {
        if (e->id() == id)
            return e;
    }
    return nullptr;
}

void GraphModel::clear()
{
    if (!m_edges.isEmpty()) {
        m_edges.clear();
        emit edgesChanged();
    }
    if (!m_nodes.isEmpty()) {
        m_nodes.clear();
        emit nodesChanged();
    }
}
