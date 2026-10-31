#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_CORE_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_CORE_H

#include "component_map_runtime/DeploymentManager.h"
#include "component_map_runtime/RuntimeHandlerRegistry.h"

#include <QHash>
#include <QList>
#include <memory>
#include <QQueue>
#include <QString>
#include <QStringList>

namespace cme::runtime
{

class RuntimePersistence;

enum class ActorStatus
{
    Idle,
    Busy,
    Failed,
    Stopped
};

struct RejectionRecord
{
    QString eventId;
    QString idempotencyKey;
    QString targetComponentId;
    QString routeId;
    QString reasonCode;
    qsizetype mailboxDepth = 0;
};

struct FailureRecord
{
    RuntimeFailure failure;
    QString reasonCode;
    QString routeId;
};

struct IdempotencyRecord
{
    QByteArray requestFingerprint;
    bool completed = false;
    bool succeeded = false;
    RuntimeFailure failure;
};

class Mailbox
{
public:
    explicit Mailbox(qsizetype capacity = 64);

    bool enqueue(const RuntimeEvent &event);
    bool dequeue(RuntimeEvent *event);
    void clear();
    qsizetype size() const;
    qsizetype capacity() const;
    bool empty() const;

private:
    qsizetype m_capacity;
    QQueue<RuntimeEvent> m_events;
};

class ComponentStateStore
{
public:
    QByteArray value(const QString &instanceId) const;
    void setValue(const QString &instanceId, const QByteArray &state);
    void clear();
    bool contains(const QString &instanceId) const;

private:
    QHash<QString, QByteArray> m_states;
};

class RuntimeEngine
{
public:
    RuntimeEngine(RuntimeService *runtimeService,
                  DeploymentManager *deploymentManager,
                  const RuntimeHandlerRegistry *handlerRegistry,
                  qsizetype mailboxCapacity = 64,
                  RuntimePersistence *persistence = nullptr);

    bool start(QString *error = nullptr);
    bool stop(QString *error = nullptr);
    bool publish(const RuntimeEvent &event, RuntimeFailure *failure = nullptr);
    qsizetype processAll();

    ActorStatus actorStatus(const QString &instanceId) const;
    qsizetype mailboxDepth(const QString &instanceId) const;
    QByteArray componentState(const QString &instanceId) const;
    const ComponentStateStore &stateStore() const;
    const QList<FailureRecord> &failureLog() const;
    const QList<RejectionRecord> &rejections() const;
    qsizetype dispatchedCount() const;
    qsizetype completedCount() const;
    qsizetype rejectedCount() const;
    bool isRunning() const;
    bool recover(QString *error = nullptr);

private:
    struct Actor
    {
        QString componentType;
        Mailbox mailbox;
        std::shared_ptr<RuntimeHandler> handler;
        ActorStatus status = ActorStatus::Stopped;
        bool queuedForScheduling = false;
    };

    bool enqueueToActor(const RuntimeEvent &event,
                        const QString &targetId,
                        const QString &routeId,
                        RuntimeFailure *failure);
    void recordRejection(const RuntimeEvent &event,
                         const QString &targetId,
                         const QString &routeId,
                         const QString &reasonCode,
                         qsizetype depth);
    void recordFailure(const RuntimeFailure &failure, const QString &routeId);
    bool dispatchFromActor(const QString &instanceId);
    QByteArray fingerprint(const RuntimeEvent &event) const;
    bool resolveActors(QString *error);

    RuntimeService *m_runtimeService = nullptr;
    DeploymentManager *m_deploymentManager = nullptr;
    const RuntimeHandlerRegistry *m_handlerRegistry = nullptr;
    RuntimePersistence *m_persistence = nullptr;
    qsizetype m_mailboxCapacity;
    QHash<QString, Actor> m_actors;
    QQueue<QString> m_readyActors;
    ComponentStateStore m_stateStore;
    QList<FailureRecord> m_failureLog;
    QList<RejectionRecord> m_rejections;
    QHash<QString, IdempotencyRecord> m_idempotency;
    qsizetype m_dispatchedCount = 0;
    qsizetype m_completedCount = 0;
    qsizetype m_rejectedCount = 0;
    bool m_running = false;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_CORE_H
