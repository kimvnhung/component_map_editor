#ifndef GRAPHMODEL_H
#define GRAPHMODEL_H

#include <QObject>
#include <QList>
#include <QVariantList>
#include <QHash>

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

    // Batch-update mode: suppresses per-item componentsChanged/connectionsChanged
    // signals and skips duplicate-ID checks inside addComponent/addConnection.
    // Call endBatchUpdate() to emit a single consolidated changed signal pair.
    Q_INVOKABLE void beginBatchUpdate();
    Q_INVOKABLE void endBatchUpdate();

signals:
    void componentAdded(ComponentModel *component);
    void componentRemoved(const QString &id);
    void connectionAdded(ConnectionModel *connection);
    void connectionRemoved(const QString &id);
    void componentsChanged();
    void connectionsChanged();

private:
    ComponentModel *firstComponentByIdLinear(const QString &id) const;
    ConnectionModel *firstConnectionByIdLinear(const QString &id) const;
    void attachComponent(ComponentModel *component);
    void detachComponent(ComponentModel *component);
    void attachConnection(ConnectionModel *connection);
    void detachConnection(ConnectionModel *connection);

    void onTrackedComponentIdChanged(ComponentModel *component);
    void onTrackedConnectionIdChanged(ConnectionModel *connection);

#ifdef QT_DEBUG
    bool verifyComponentIndexIntegrity() const;
    bool verifyConnectionIndexIntegrity() const;
    void assertIndexIntegrity() const;
#endif

    QList<ComponentModel *> m_components;
    QList<ConnectionModel *> m_connections;
    QHash<QString, ComponentModel *> m_componentIndex;
    QHash<QString, ConnectionModel *> m_connectionIndex;
    QHash<ComponentModel *, QString> m_componentTrackedIds;
    QHash<ConnectionModel *, QString> m_connectionTrackedIds;
    bool m_batchMode = false;
};

#endif // GRAPHMODEL_H
