#ifndef GRAPHEXECUTIONSANDBOX_H
#define GRAPHEXECUTIONSANDBOX_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QQmlEngine>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/GraphModel.h"

class ExtensionContractRegistry;

class GraphExecutionSandbox : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel *graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(int currentTick READ currentTick NOTIFY currentTickChanged FINAL)
    Q_PROPERTY(QVariantList timeline READ timeline NOTIFY timelineChanged FINAL)
    Q_PROPERTY(QVariantMap executionState READ executionState NOTIFY executionStateChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)

public:
    explicit GraphExecutionSandbox(QObject *parent = nullptr);

    GraphModel *graph() const;
    void setGraph(GraphModel *graph);

    QString status() const;
    int currentTick() const;
    QVariantList timeline() const;
    QVariantMap executionState() const;
    QString lastError() const;

    void setExecutionSemanticsProviders(const QList<const IExecutionSemanticsProvider *> &providers);
    void rebuildSemanticsFromRegistry(const ExtensionContractRegistry &registry);

    Q_INVOKABLE bool start(const QVariantMap &inputSnapshot = {});
    Q_INVOKABLE bool step();
    Q_INVOKABLE int run(int maxSteps = -1);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void reset();

    Q_INVOKABLE void setBreakpoint(const QString &componentId, bool enabled = true);
    Q_INVOKABLE void clearBreakpoints();
    Q_INVOKABLE QStringList breakpoints() const;

    Q_INVOKABLE QVariantMap componentState(const QString &componentId) const;
    Q_INVOKABLE QVariantMap snapshotSummary() const;

signals:
    void graphChanged();
    void statusChanged();
    void currentTickChanged();
    void timelineChanged();
    void executionStateChanged();
    void lastErrorChanged();

private:
    struct ComponentSnapshot {
        QString id;
        QString type;
        QString title;
        QVariantMap attributes;
    };

    struct ConnectionSnapshot {
        QString id;
        QString sourceId;
        QString targetId;
        QString label;
    };

    enum class RunStatus {
        Idle,
        Running,
        Paused,
        Completed,
        Error
    };

    static QString statusToString(RunStatus status);

    void setStatus(RunStatus status);
    void appendTimelineEvent(const QString &event, const QVariantMap &payload = {});
    void markError(const QString &message);

    void clearSimulationData();
    bool captureGraphSnapshot();
    bool executeOneStep(bool bypassBreakpoint);
    void finalizeIfNoReadyComponents();
    void flushTimelineChanged();

    void enqueueReadyComponent(const QString &componentId);
    QVariantMap toComponentSnapshotMap(const ComponentSnapshot &component) const;

    QPointer<GraphModel> m_graph;
    RunStatus m_status = RunStatus::Idle;
    int m_tick = 0;

    QVariantMap m_inputSnapshot;
    QVariantMap m_executionState;
    QVariantMap m_componentStates;
    QVariantList m_timeline;
    QString m_lastError;

    QHash<QString, ComponentSnapshot> m_componentsById;
    QHash<QString, QList<ConnectionSnapshot>> m_outgoingBySource;
    QHash<QString, int> m_pendingInDegree;
    QSet<QString> m_executed;
    QStringList m_readyQueue;
    QSet<QString> m_readyQueueSet;

    bool m_deferTimelineSignal = false;
    bool m_timelineDirty = false;

    QSet<QString> m_breakpoints;
    QHash<QString, const IExecutionSemanticsProvider *> m_providerByComponentType;
};

#endif // GRAPHEXECUTIONSANDBOX_H
