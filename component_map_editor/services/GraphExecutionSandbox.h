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

#include <memory>

#include <google/protobuf/struct.pb.h>
#include <execution.pb.h>

#include "models/GraphModel.h"
#include "models/TimelineModel.h"

#include "services/execution_internal/ExecutionContext.h"
#include "services/execution_internal/IExecutionEngine.h"

class ExtensionContractRegistry;

class GraphExecutionSandbox : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(GraphModel *graph READ graph WRITE setGraph NOTIFY graphChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(int currentTick READ currentTick NOTIFY currentTickChanged FINAL)
    Q_PROPERTY(TimelineModel *timeline READ timeline CONSTANT FINAL)
    Q_PROPERTY(QVariantMap executionState READ executionState NOTIFY executionStateChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)
    Q_PROPERTY(QVariantMap providerOutputKeyHints READ providerOutputKeyHints NOTIFY providerOutputKeyHintsChanged FINAL)


public:
    explicit GraphExecutionSandbox(QObject *parent = nullptr);

    GraphModel *graph() const;
    void setGraph(GraphModel *graph);

    QString status() const;
    int currentTick() const;
    TimelineModel *timeline() const;
    QVariantMap executionState() const;
    QString lastError() const;

    QVariantMap providerOutputKeyHints() const;

    void setExecutionSemanticsProviders(const QList<const IExecutionSemanticsProvider *> &providers);
    void rebuildSemanticsFromRegistry(const ExtensionContractRegistry &registry);

    // Internal-only (not QML-invokable): forces SequentialExecutionEngine regardless of the
    // actor-engine flag. Used by CompositeExecutionProvider for nested sandboxes so a composite
    // component never spins up its own worker thread pool from inside an actor-engine step.
    void forceSequentialEngineForNestedExecution();

    // Legacy wrapper for QML/internal map-based start.
    Q_INVOKABLE bool start(const QVariantMap &inputSnapshot = {});
    // Typed external entrypoint. Preferred for integrations outside the library.
    bool startTyped(const google::protobuf::Struct &inputSnapshot);
    Q_INVOKABLE bool step();
    Q_INVOKABLE int run(int maxSteps = -1);
    Q_INVOKABLE void pause();
    Q_INVOKABLE void reset();

    Q_INVOKABLE void setBreakpoint(const QString &componentId, bool enabled = true);
    Q_INVOKABLE void clearBreakpoints();
    Q_INVOKABLE QStringList breakpoints() const;

    // Legacy wrapper for QML/internal map-based state access.
    Q_INVOKABLE QVariantMap componentState(const QString &componentId) const;
    bool componentStateTyped(const QString &componentId,
                             google::protobuf::Struct *outState,
                             QString *error = nullptr) const;
    Q_INVOKABLE QVariantMap snapshotSummary() const;
    Q_INVOKABLE QVariantMap debugSnapshot() const;
    Q_INVOKABLE QVariantMap executionTelemetry() const;
    Q_INVOKABLE QStringList sensitiveDebugKeys() const;
    Q_INVOKABLE void setSensitiveDebugKeys(const QStringList &keys);

    cme::ExecutionSnapshot executionSnapshotTyped() const;

signals:
    void graphChanged();
    void statusChanged();
    void currentTickChanged();
    void timelineChanged();
    void executionStateChanged();
    void lastErrorChanged();
    void providerOutputKeyHintsChanged();

private:
    struct ComponentSnapshot
    {
        QString id;
        QString type;
        QString title;
        QVariantMap attributes;
    };

    struct ConnectionSnapshot
    {
        QString id;
        QString sourceId;
        QString targetId;
        QString label;
    };

    enum class RunStatus
    {
        Idle,
        Running,
        Paused,
        Completed,
        Error
    };

    enum class TimelineEventKind
    {
        SimulationStarted,
        StepExecuted,
        SimulationPaused,
        SimulationCompleted,
        SimulationBlocked,
        BreakpointHit,
        Error
    };

    static QString statusToString(RunStatus status);
    static cme::TimelineEventType timelineKindToProtoType(TimelineEventKind kind);

    void setStatus(RunStatus status);
    void appendTimelineEvent(TimelineEventKind kind, const QVariantMap &payload = {});
    void markError(const QString &message);

    void clearSimulationData();
    bool executeOneStep(bool bypassBreakpoint);
    // Functions for making executionOneStep more clearly

    // Picks SequentialExecutionEngine vs ActorMailboxExecutionEngine based on
    // MigrationFlags::actorEngineEnabled()/tokenTransportEnabled() and whether breakpoints are
    // set (actor engine does not support deterministic breakpoint gating - see ADR Plan 3).
    // Only re-evaluated at start(); changing breakpoints/flags mid-run does not swap engines.
    void selectEngine();

    // Áp phần state thuộc public API (componentStates/tick/timeline) từ kết quả engine trả về.
    void commitExecutionState(const ExecutionContext& ctx, const ExecuteResult& result);
    void recordTimelineEvent(const ExecutionContext& ctx, const ExecuteResult& result);

    //
    void finalizeIfNoReadyComponents();
    void flushTimelineChanged();

private:
    std::unique_ptr<IExecutionEngine> m_engine;

    QPointer<GraphModel> m_graph;
    RunStatus m_status = RunStatus::Idle;
    int m_tick = 0;

    QVariantMap m_executionState;
    QVariantMap m_componentStates;
    TimelineModel *m_timeline;
    QList<cme::TimelineEvent> m_typedTimeline;
    QString m_lastError;

    bool m_deferTimelineSignal = false;
    bool m_timelineDirty = false;

    bool m_usingActorEngine = false;
    bool m_forceSequentialEngine = false;

    int m_redactedFieldCount = 0;

    QSet<QString> m_breakpoints;
    QSet<QString> m_sensitiveDebugKeys;
    QHash<QString, const IExecutionSemanticsProvider *> m_providerByComponentType;
};

#endif // GRAPHEXECUTIONSANDBOX_H
