#ifndef TOKENKEYCATALOG_H
#define TOKENKEYCATALOG_H

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>

#include "models/GraphModel.h"

class TokenKeyCatalog : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel *graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(QVariantMap providerOutputKeyHints READ providerOutputKeyHints
               WRITE setProviderOutputKeyHints NOTIFY providerOutputKeyHintsChanged FINAL)
    Q_PROPERTY(QString targetComponentId READ targetComponentId
               WRITE setTargetComponentId NOTIFY targetComponentIdChanged FINAL)
    Q_PROPERTY(QStringList tokenKeys READ tokenKeys NOTIFY tokenKeysChanged FINAL)
    Q_PROPERTY(QVariantList tokenKeyOptions READ tokenKeyOptions NOTIFY tokenKeyOptionsChanged FINAL)

public:
    explicit TokenKeyCatalog(QObject *parent = nullptr);

    GraphModel *graph() const;
    void setGraph(GraphModel *graph);

    QVariantMap providerOutputKeyHints() const;
    void setProviderOutputKeyHints(const QVariantMap &hints);

    QString targetComponentId() const;
    void setTargetComponentId(const QString &targetComponentId);

    QStringList tokenKeys() const;
    QVariantList tokenKeyOptions() const;

    Q_INVOKABLE void refresh();

signals:
    void graphChanged();
    void providerOutputKeyHintsChanged();
    void targetComponentIdChanged();
    void tokenKeysChanged();
    void tokenKeyOptionsChanged();

private:
    void reconnectGraphSignals();
    static QVariantList sortedOptionsByText(const QVariantList &rows);
    void publishOptions(const QStringList &keys, const QVariantList &options);

    QPointer<GraphModel> m_graph;
    QVariantMap m_providerOutputKeyHints;
    QString m_targetComponentId;
    QStringList m_tokenKeys;
    QVariantList m_tokenKeyOptions;
};

#endif // TOKENKEYCATALOG_H
