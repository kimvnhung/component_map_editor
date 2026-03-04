#ifndef GRAPHMODEL_H
#define GRAPHMODEL_H

#include <QObject>
#include <QList>
#include <QVariantList>

#include "ComponentModel.h"
#include "ConnectionModel.h"

class GraphModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList components READ componentsVariant NOTIFY componentsChanged FINAL)
    Q_PROPERTY(QVariantList connections READ connectionsVariant NOTIFY connectionsChanged FINAL)
    Q_PROPERTY(int componentCount READ componentCount NOTIFY componentsChanged FINAL)
    Q_PROPERTY(int connectionCount READ connectionCount NOTIFY connectionsChanged FINAL)

public:
    explicit GraphModel(QObject *parent = nullptr);

    QVariantList componentsVariant() const;
    QVariantList connectionsVariant() const;

    int componentCount() const;
    int connectionCount() const;

    const QList<ComponentModel *> &componentList() const;
    const QList<ConnectionModel *> &connectionList() const;

    Q_INVOKABLE void addComponent(ComponentModel *component);
    Q_INVOKABLE bool removeComponent(const QString &id);
    Q_INVOKABLE ComponentModel *componentById(const QString &id) const;

    Q_INVOKABLE void addConnection(ConnectionModel *connection);
    Q_INVOKABLE bool removeConnection(const QString &id);
    Q_INVOKABLE ConnectionModel *connectionById(const QString &id) const;

    Q_INVOKABLE void clear();

signals:
    void componentAdded(ComponentModel *component);
    void componentRemoved(const QString &id);
    void connectionAdded(ConnectionModel *connection);
    void connectionRemoved(const QString &id);
    void componentsChanged();
    void connectionsChanged();

private:
    QList<ComponentModel *> m_components;
    QList<ConnectionModel *> m_connections;
};

#endif // GRAPHMODEL_H
