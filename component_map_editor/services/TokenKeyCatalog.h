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
    Q_PROPERTY(QString targetComponentId READ targetComponentId
               WRITE setTargetComponentId NOTIFY targetComponentIdChanged FINAL)
    Q_PROPERTY(QStringList tokenKeys READ tokenKeys NOTIFY tokenKeysChanged FINAL)
    Q_PROPERTY(QVariantList tokenKeyOptions READ tokenKeyOptions NOTIFY tokenKeyOptionsChanged FINAL)

public:
    explicit TokenKeyCatalog(QObject *parent = nullptr);

    GraphModel *graph() const;
    void setGraph(GraphModel *graph);

    QVariantMap executionStateSnapshot() const;
    void setExecutionStateSnapshot(const QVariantMap &executionStateSnapshot);

    QVariantList schemaSections() const;
    void setSchemaSections(const QVariantList &schemaSections);

    QString targetComponentId() const;
    void setTargetComponentId(const QString &targetComponentId);

    QStringList tokenKeys() const;
    QVariantList tokenKeyOptions() const;

    Q_INVOKABLE void refresh();

signals:
    void graphChanged();
    void executionStateSnapshotChanged();
    void schemaSectionsChanged();
    void targetComponentIdChanged();
    void tokenKeysChanged();
    void tokenKeyOptionsChanged();

private:
    struct TokenOptionAccumulator;

    void reconnectGraphSignals();
    void rebuildConnectionTracking();

    QHash<QString, QString> incomingSourceByTokenId() const;
    bool collectRuntimeOptions(const QHash<QString, QString> &sourceByIncomingTokenId,
                               TokenOptionAccumulator *accumulator) const;
    void collectInputStateFallback(TokenOptionAccumulator *accumulator,
                                   bool *hasRuntimeIncomingTokenKeys) const;
    void collectConsumedTokenIdFallback(TokenOptionAccumulator *accumulator,
                                        bool *hasRuntimeIncomingTokenKeys) const;
    void collectConnectionFallback(TokenOptionAccumulator *accumulator,
                                   bool hasRuntimeIncomingTokenKeys) const;
    static QStringList sortedKeys(const QSet<QString> &keys);
    static QVariantList sortedOptionsByText(const QVariantList &rows);
    void publishOptions(const QStringList &keys, const QVariantList &options);

    static void addTokenKeyCandidate(const QVariant &candidate, QSet<QString> *out);

    QPointer<GraphModel> m_graph;
    QVariantMap m_executionStateSnapshot;
    QVariantList m_schemaSections;
    QString m_targetComponentId;
    QStringList m_tokenKeys;
    QVariantList m_tokenKeyOptions;
    QList<ConnectionModel *> m_trackedConnections;
};

#endif // TOKENKEYCATALOG_H
