#ifndef CME_ACTOR_H
#define CME_ACTOR_H

#include <atomic>
#include <functional>
#include <memory>

#include "Mailbox.h"
#include "../ExecutionContext.h"

namespace cme::actor
{

using ActorFinishedCallback = std::function<void(const ExecutionContext &ctx, const ExecuteResult &result)>;

class IActor
{
public:
    IActor(const QString &id, std::unique_ptr<IMailbox> mailbox);
    virtual ~IActor() = default;

    // Processes one message. Called by the scheduler's worker thread.
    virtual void onMessage(Message &&msg) = 0;

    QString id() const { return m_id; }
    bool hasWork() const { return !m_mailbox->empty(); }

    // Atomically claims "this actor is now in the run-queue" so the scheduler never
    // double-schedules the same actor from two workers at once.
    bool enqueuedIfNot();
    void markNotEnqueued();

    bool enqueueMessage(Message &&msg) { return m_mailbox->enqueue(std::move(msg)); }
    IMailbox &mailbox() { return *m_mailbox; }

private:
    QString m_id;
    std::unique_ptr<IMailbox> m_mailbox;
    std::atomic_bool m_enqueued{false};
};

// Actor wrapping a single graph component + its resolved execution semantics provider.
// Mirrors SequentialExecutionEngine::invokeProvider()/validateExecutionResult() so both
// engines produce the same ExecuteResult shape for a given (componentType, tokens) input.
class ComponentActor : public IActor
{
public:
    ComponentActor(const QString &id,
                   const QString &componentType,
                   const cme::ComponentData &componentData,
                   const IExecutionSemanticsProvider *provider,
                   ActorFinishedCallback onFinished,
                   std::unique_ptr<IMailbox> mailbox = std::make_unique<Mailbox>());

    void onMessage(Message &&msg) override;

    QString componentType() const { return m_componentType; }

private:
    QString m_componentType;
    cme::ComponentData m_componentData;
    const IExecutionSemanticsProvider *m_provider;
    ActorFinishedCallback m_onFinished;
};

} // namespace cme::actor

#endif // CME_ACTOR_H
