#pragma once

#include <QObject>
#include <QList>
#include <QVariantList>
#include <qqml.h>

#include "NodeModel.h"
#include "EdgeModel.h"

class GraphModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList nodes READ nodesVariant NOTIFY nodesChanged FINAL)
    Q_PROPERTY(QVariantList edges READ edgesVariant NOTIFY edgesChanged FINAL)
    Q_PROPERTY(int nodeCount READ nodeCount NOTIFY nodesChanged FINAL)
    Q_PROPERTY(int edgeCount READ edgeCount NOTIFY edgesChanged FINAL)

public:
    explicit GraphModel(QObject *parent = nullptr);

    QVariantList nodesVariant() const;
    QVariantList edgesVariant() const;

    int nodeCount() const;
    int edgeCount() const;

    const QList<NodeModel *> &nodeList() const;
    const QList<EdgeModel *> &edgeList() const;

    Q_INVOKABLE void addNode(NodeModel *node);
    Q_INVOKABLE bool removeNode(const QString &id);
    Q_INVOKABLE NodeModel *nodeById(const QString &id) const;

    Q_INVOKABLE void addEdge(EdgeModel *edge);
    Q_INVOKABLE bool removeEdge(const QString &id);
    Q_INVOKABLE EdgeModel *edgeById(const QString &id) const;

    Q_INVOKABLE void clear();

signals:
    void nodeAdded(NodeModel *node);
    void nodeRemoved(const QString &id);
    void edgeAdded(EdgeModel *edge);
    void edgeRemoved(const QString &id);
    void nodesChanged();
    void edgesChanged();

private:
    QList<NodeModel *> m_nodes;
    QList<EdgeModel *> m_edges;
};
