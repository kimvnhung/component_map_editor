#include "component_map_runtime/RuntimeCore.h"
#include "component_map_runtime/RuntimePersistence.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QIODevice>
#include <utility>

namespace
{
using namespace cme::runtime;
using namespace cme::runtime::v1;

void setFailure(RuntimeFailure *failure, const QString &code, const QString &message,
                const RuntimeEvent &event, const QString &componentId)
{
    if (!failure)
    {
        return;
    }
    failure->code = code;
    failure->message = message;
    failure->eventId = event.eventId;
    failure->componentId = componentId;
}
}

namespace cme::runtime
{

Mailbox::Mailbox(qsizetype capacity)
    : m_capacity(qMax<qsizetype>(0, capacity))
{
}

bool Mailbox::enqueue(const RuntimeEvent &event)
{
    if (m_events.size() >= m_capacity)
    {
        return false;
    }
    m_events.enqueue(event);
    return true;
}

bool Mailbox::dequeue(RuntimeEvent *event)
{
    if (!event || m_events.isEmpty())
    {
        return false;
    }
    *event = m_events.dequeue();
    return true;
}

void Mailbox::clear()
{
    m_events.clear();
}

qsizetype Mailbox::size() const
{
    return m_events.size();
}

qsizetype Mailbox::capacity() const
{
    return m_capacity;
}

bool Mailbox::empty() const
{
    return m_events.isEmpty();
}

QByteArray ComponentStateStore::value(const QString &instanceId) const
{
    return m_states.value(instanceId);
}

void ComponentStateStore::setValue(const QString &instanceId, const QByteArray &state)
{
    m_states.insert(instanceId, state);
}

void ComponentStateStore::clear()
{
    m_states.clear();
}

bool ComponentStateStore::contains(const QString &instanceId) const
{
    return m_states.contains(instanceId);
}

RuntimeEngine::RuntimeEngine(RuntimeService *runtimeService,
                             DeploymentManager *deploymentManager,
                             const RuntimeHandlerRegistry *handlerRegistry,
                             qsizetype mailboxCapacity,
                             RuntimePersistence *persistence)
    : m_runtimeService(runtimeService)
    , m_deploymentManager(deploymentManager)
    , m_handlerRegistry(handlerRegistry)
    , m_mailboxCapacity(mailboxCapacity)
    , m_persistence(persistence)
{
}

bool RuntimeEngine::start(QString *error)
{
    if (!m_runtimeService || !m_deploymentManager || !m_handlerRegistry)
    {
        if (error)
        {
            *error = QStringLiteral("Runtime dependencies are required.");
        }
        return false;
    }
    if (!m_deploymentManager->hasActiveGraph())
    {
        if (error)
        {
            *error = QStringLiteral("An active graph is required before starting.");
        }
        return false;
    }
    if (m_running)
    {
        if (error)
        {
            error->clear();
        }
        return true;
    }
    if (!m_runtimeService->start(error))
    {
        return false;
    }
    if (!resolveActors(error))
    {
        m_runtimeService->stop();
        return false;
    }
    if (m_persistence)
    {
        if (!m_persistence->saveGraph(m_deploymentManager->activeGraph(), error)
            || !recover(error))
        {
            m_runtimeService->stop();
            return false;
        }
    }
    m_running = true;
    if (error)
    {
        error->clear();
    }
    return true;
}

bool RuntimeEngine::stop(QString *error)
{
    if (!m_running)
    {
        if (m_runtimeService && m_runtimeService->status() != v1::RUNTIME_STATUS_STOPPED)
        {
            return m_runtimeService->stop(error);
        }
        if (error)
        {
            error->clear();
        }
        return true;
    }
    m_running = false;
    if (m_persistence)
    {
        for (auto it = m_actors.cbegin(); it != m_actors.cend(); ++it)
        {
            m_persistence->saveComponentState(it.key(), it->componentType,
                                              m_stateStore.value(it.key()), 1);
        }
    }
    m_readyActors.clear();
    for (auto it = m_actors.begin(); it != m_actors.end(); ++it)
    {
        it.value().mailbox.clear();
        it.value().queuedForScheduling = false;
        it.value().status = ActorStatus::Stopped;
    }
    return m_runtimeService->stop(error);
}

bool RuntimeEngine::publish(const RuntimeEvent &event, RuntimeFailure *failure)
{
    if (!m_running)
    {
        setFailure(failure, QStringLiteral("RUNTIME_NOT_RUNNING"),
                   QStringLiteral("Runtime is not running."), event, event.targetComponentId);
        recordRejection(event, event.targetComponentId, QString(), QStringLiteral("RUNTIME_NOT_RUNNING"), 0);
        return false;
    }
    if (event.eventId.isEmpty() || event.targetComponentId.isEmpty())
    {
        setFailure(failure, QStringLiteral("INVALID_EVENT"),
                   QStringLiteral("Event ID and target are required."), event, event.targetComponentId);
        recordRejection(event, event.targetComponentId, QString(), QStringLiteral("INVALID_EVENT"), 0);
        return false;
    }
    if (!event.idempotencyKey.isEmpty())
    {
        const QByteArray currentFingerprint = fingerprint(event);
        const auto existing = m_idempotency.constFind(event.idempotencyKey);
        if (existing != m_idempotency.cend())
        {
            if (existing->requestFingerprint != currentFingerprint)
            {
                setFailure(failure, QStringLiteral("IDEMPOTENCY_CONFLICT"),
                           QStringLiteral("Idempotency key has a different request fingerprint."),
                           event, event.targetComponentId);
                recordRejection(event, event.targetComponentId, QString(),
                                QStringLiteral("IDEMPOTENCY_CONFLICT"), 0);
                return false;
            }
            if (!existing->completed)
            {
                setFailure(failure, QStringLiteral("IDEMPOTENCY_IN_FLIGHT"),
                           QStringLiteral("Idempotency key is already being processed."),
                           event, event.targetComponentId);
                recordRejection(event, event.targetComponentId, QString(),
                                QStringLiteral("IDEMPOTENCY_IN_FLIGHT"), 0);
                return false;
            }
            if (!existing->succeeded && failure)
            {
                *failure = existing->failure;
            }
            return existing->succeeded;
        }
        m_idempotency.insert(event.idempotencyKey, {currentFingerprint, false, false, {}});
    }

    ++m_dispatchedCount;
    const bool accepted = enqueueToActor(event, event.targetComponentId, QString(), failure);
    if (accepted && m_persistence)
    {
        QString persistenceError;
        if (!m_persistence->recordAcceptedEvent(event, &persistenceError))
        {
            setFailure(failure, QStringLiteral("PERSISTENCE_FAILURE"), persistenceError,
                       event, event.targetComponentId);
            return false;
        }
    }
    if (!accepted && !event.idempotencyKey.isEmpty())
    {
        auto it = m_idempotency.find(event.idempotencyKey);
        if (it != m_idempotency.end())
        {
            it->completed = true;
            it->succeeded = false;
            it->failure = failure ? *failure : RuntimeFailure{};
        }
    }
    return accepted;
}

qsizetype RuntimeEngine::processAll()
{
    qsizetype processed = 0;
    while (!m_readyActors.isEmpty())
    {
        const QString actorId = m_readyActors.dequeue();
        auto actorIt = m_actors.find(actorId);
        if (actorIt == m_actors.end())
        {
            continue;
        }
        actorIt->queuedForScheduling = false;
        if (dispatchFromActor(actorId))
        {
            ++processed;
        }
    }
    return processed;
}

ActorStatus RuntimeEngine::actorStatus(const QString &instanceId) const
{
    const auto it = m_actors.constFind(instanceId);
    return it == m_actors.cend() ? ActorStatus::Stopped : it->status;
}

qsizetype RuntimeEngine::mailboxDepth(const QString &instanceId) const
{
    const auto it = m_actors.constFind(instanceId);
    return it == m_actors.cend() ? 0 : it->mailbox.size();
}

bool RuntimeEngine::recover(QString *error)
{
    if (!m_persistence || !m_persistence->isOpen())
    {
        if (error) *error = QStringLiteral("Persistence is not open.");
        return false;
    }
    const auto states = m_persistence->loadComponentStates(error);
    if (error && !error->isEmpty()) return false;
    for (auto it = states.cbegin(); it != states.cend(); ++it)
    {
        if (m_actors.contains(it.key())) m_stateStore.setValue(it.key(), it.value());
    }
    return true;
}

QByteArray RuntimeEngine::componentState(const QString &instanceId) const
{
    return m_stateStore.value(instanceId);
}

const ComponentStateStore &RuntimeEngine::stateStore() const
{
    return m_stateStore;
}

const QList<FailureRecord> &RuntimeEngine::failureLog() const
{
    return m_failureLog;
}

const QList<RejectionRecord> &RuntimeEngine::rejections() const
{
    return m_rejections;
}

qsizetype RuntimeEngine::dispatchedCount() const
{
    return m_dispatchedCount;
}

qsizetype RuntimeEngine::completedCount() const
{
    return m_completedCount;
}

qsizetype RuntimeEngine::rejectedCount() const
{
    return m_rejectedCount;
}

bool RuntimeEngine::isRunning() const
{
    return m_running;
}

bool RuntimeEngine::enqueueToActor(const RuntimeEvent &event,
                                   const QString &targetId,
                                   const QString &routeId,
                                   RuntimeFailure *failure)
{
    auto actorIt = m_actors.find(targetId);
    if (actorIt == m_actors.end())
    {
        setFailure(failure, QStringLiteral("DELIVERY_TARGET_NOT_FOUND"),
                   QStringLiteral("Target actor does not exist."), event, targetId);
        recordFailure(failure ? *failure : RuntimeFailure{}, routeId);
        recordRejection(event, targetId, routeId, QStringLiteral("DELIVERY_TARGET_NOT_FOUND"), 0);
        return false;
    }
    if (!actorIt->mailbox.enqueue(event))
    {
        setFailure(failure, QStringLiteral("MAILBOX_FULL"),
                   QStringLiteral("Target mailbox is full."), event, targetId);
        recordRejection(event, targetId, routeId, QStringLiteral("MAILBOX_FULL"),
                        actorIt->mailbox.size());
        return false;
    }
    if (!actorIt->queuedForScheduling)
    {
        actorIt->queuedForScheduling = true;
        m_readyActors.enqueue(targetId);
    }
    return true;
}

void RuntimeEngine::recordRejection(const RuntimeEvent &event,
                                    const QString &targetId,
                                    const QString &routeId,
                                    const QString &reasonCode,
                                    qsizetype depth)
{
    const RejectionRecord record{event.eventId, event.idempotencyKey, targetId, routeId, reasonCode, depth};
    m_rejections.append(record);
    if (m_persistence)
    {
        m_persistence->recordRejection(record);
        m_persistence->recordLog(QStringLiteral("warning"), reasonCode,
                                 QStringLiteral("Runtime event rejected."));
    }
    ++m_rejectedCount;
}

void RuntimeEngine::recordFailure(const RuntimeFailure &failure, const QString &routeId)
{
    m_failureLog.append({failure, failure.code, routeId});
}

bool RuntimeEngine::dispatchFromActor(const QString &instanceId)
{
    auto actorIt = m_actors.find(instanceId);
    if (actorIt == m_actors.end())
    {
        return false;
    }
    RuntimeEvent event;
    if (!actorIt->mailbox.dequeue(&event))
    {
        actorIt->status = ActorStatus::Idle;
        return false;
    }
    actorIt->status = ActorStatus::Busy;
    RuntimeEvent output;
    RuntimeFailure failure;
    QByteArray state = m_stateStore.value(instanceId);
    ++m_dispatchedCount;
    if (!actorIt->handler->process(event, state, &output, &failure))
    {
        actorIt->status = ActorStatus::Failed;
        if (failure.code.isEmpty())
        {
            failure.code = QStringLiteral("HANDLER_FAILURE");
        }
        if (failure.eventId.isEmpty())
        {
            failure.eventId = event.eventId;
        }
        if (failure.componentId.isEmpty())
        {
            failure.componentId = instanceId;
        }
        recordFailure(failure, QString());
        if (m_persistence)
        {
            m_persistence->recordEventCompletion(event.eventId, false, failure);
        }
        if (!event.idempotencyKey.isEmpty())
        {
            auto idempotencyIt = m_idempotency.find(event.idempotencyKey);
            if (idempotencyIt != m_idempotency.end())
            {
                idempotencyIt->completed = true;
                idempotencyIt->succeeded = false;
                idempotencyIt->failure = failure;
            }
        }
        return true;
    }
    m_stateStore.setValue(instanceId, state);

    const auto &graph = m_deploymentManager->activeGraph();
    for (const auto &route : graph.routes())
    {
        if (!route.enabled()
            || QString::fromStdString(route.source_instance_id()) != instanceId)
        {
            continue;
        }
        if (QString::fromStdString(route.message_type()) != output.messageType)
        {
            RuntimeFailure routeFailure;
            setFailure(&routeFailure, QStringLiteral("DELIVERY_MESSAGE_TYPE_MISMATCH"),
                       QStringLiteral("Event message type does not match route."), output,
                       QString::fromStdString(route.target_instance_id()));
            recordFailure(routeFailure, QString::fromStdString(route.connection_id()));
            continue;
        }
        RuntimeFailure deliveryFailure;
        enqueueToActor(output, QString::fromStdString(route.target_instance_id()),
                       QString::fromStdString(route.connection_id()), &deliveryFailure);
    }
    actorIt->status = actorIt->mailbox.empty() ? ActorStatus::Idle : ActorStatus::Busy;
    if (!actorIt->mailbox.empty() && !actorIt->queuedForScheduling)
    {
        actorIt->queuedForScheduling = true;
        m_readyActors.enqueue(instanceId);
    }
    ++m_completedCount;
    if (m_persistence)
    {
        m_persistence->recordEventCompletion(event.eventId, true);
    }
    if (!event.idempotencyKey.isEmpty())
    {
        auto idempotencyIt = m_idempotency.find(event.idempotencyKey);
        if (idempotencyIt != m_idempotency.end())
        {
            idempotencyIt->completed = true;
            idempotencyIt->succeeded = true;
        }
    }
    return true;
}

QByteArray RuntimeEngine::fingerprint(const RuntimeEvent &event) const
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << event.eventId << event.messageType << event.sourceComponentId
           << event.targetComponentId << event.payload;
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
}

bool RuntimeEngine::resolveActors(QString *error)
{
    m_actors.clear();
    m_readyActors.clear();
    m_stateStore.clear();
    const auto &graph = m_deploymentManager->activeGraph();
    for (const auto &component : graph.components())
    {
        const QString instanceId = QString::fromStdString(component.instance_id());
        auto handler = m_handlerRegistry->create(QString::fromStdString(component.component_type()));
        if (!handler)
        {
            if (error)
            {
                *error = QStringLiteral("Handler not found for component type: %1")
                             .arg(QString::fromStdString(component.component_type()));
            }
            m_actors.clear();
            return false;
        }
        Actor actor{QString::fromStdString(component.component_type()),
                    Mailbox(m_mailboxCapacity),
                    std::shared_ptr<RuntimeHandler>(std::move(handler)),
                    ActorStatus::Idle,
                    false};
        m_actors.insert(instanceId, std::move(actor));
    }
    return true;
}

} // namespace cme::runtime
