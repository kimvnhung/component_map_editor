#ifndef TOKENKEYCATALOG_H
#define TOKENKEYCATALOG_H

#include <QObject>
#include <QPointer>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>

#include "models/GraphModel.h"

class ConnectionModel;

class TokenKeyCatalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel *graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(QVariantMap executionStateSnapshot READ executionStateSnapshot
               WRITE setExecutionStateSnapshot NOTIFY executionStateSnapshotChanged FINAL)
    Q_PROPERTY(QVariantList schemaSections READ schemaSections
               WRITE setSchemaSections NOTIFY schemaSectionsChanged FINAL)
    Q_PROPERTY(QStringList tokenKeys READ tokenKeys NOTIFY tokenKeysChanged FINAL)

public:
    explicit TokenKeyCatalog(QObject *parent = nullptr);

    GraphModel *graph() const;
    void setGraph(GraphModel *graph);

    QVariantMap executionStateSnapshot() const;
    void setExecutionStateSnapshot(const QVariantMap &executionStateSnapshot);

    QVariantList schemaSections() const;
    void setSchemaSections(const QVariantList &schemaSections);

    QStringList tokenKeys() const;

    Q_INVOKABLE void refresh();

signals:
    void graphChanged();
    void executionStateSnapshotChanged();
    void schemaSectionsChanged();
    void tokenKeysChanged();

private:
    void reconnectGraphSignals();
    void rebuildConnectionTracking();

    static void addTokenKeyCandidate(const QVariant &candidate, QSet<QString> *out);

    QPointer<GraphModel> m_graph;
    QVariantMap m_executionStateSnapshot;
    QVariantList m_schemaSections;
    QStringList m_tokenKeys;
    QList<ConnectionModel *> m_trackedConnections;
};

#endif // TOKENKEYCATALOG_H
