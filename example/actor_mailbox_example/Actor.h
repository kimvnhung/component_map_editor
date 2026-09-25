#ifndef ACTOR_H
#define ACTOR_H

#include <QStringList>

#include "Mailbox.h"
#include "Component.h"
#include <atomic>
#include <memory>

class ActorScheduler;  // forward decl
class ExecutionContext;
class ExecuteResult;
class ExecutionSnapshot;

class ActorStateHistory
{
public:
    void recordState(const ExecutionContext& ctx, const ExecuteResult& result, const QVariantMap& outputTokens);
    std::vector<ExecutionSnapshot> recentHistory(size_t count = 1) const;
    std::vector<ExecutionSnapshot> history() const;
private:
    std::vector<ExecutionSnapshot> history_;
};

class IActor
{
public:
    IActor(const QString& id, std::unique_ptr<IMailbox> mailbox, ActorScheduler* scheduler);
    virtual ~IActor() = default;

    // Process one message (called by scheduler worker)
    virtual void onMessage(Message&& msg) = 0;

    // Query: does this actor have pending work?
    bool hasWork() const;

    // Get actor ID
    QString getId() const;


    // Atomically check if actor is already enqueued; if not, mark as enqueued and return true
    bool enqueuedIfNot();

    void markNotEnqueued();

    // Enqueue a message (convenience; delegates to mailbox + scheduler)
    void enqueueMessage(Message&& msg);

    // Get mailbox (for scheduler and testing)
    IMailbox &getMailbox();
    ActorScheduler *getScheduler() const;
    ActorStateHistory *stateHistory() const;

private:
    QString id_;
    std::unique_ptr<IMailbox> mailbox_;
    ActorScheduler *scheduler_;  // not owned; can be null (no auto-schedule)
    std::atomic_bool enqueued_{false};  // atomic flag: is actor already in run-queue?
    std::unique_ptr<ActorStateHistory> stateHistory_;
};

// Concrete impl: Component-based actor
class ComponentActor : public IActor
{
public:
    ComponentActor(const QString& id,
                   Component* component,
                   ActorScheduler* scheduler = nullptr,
                   QMap<QString, QStringList> connectionRoutingTable = {});

    ~ComponentActor() = default;

    // IActor impl
    void onMessage(Message&& msg) override;

    // Access component (for testing/debugging)
    Component *getComponent() const { return component_; }

private:
    Component *component_{nullptr};  // not owned
};

#endif // ACTOR_H
