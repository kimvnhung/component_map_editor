#ifndef ACTOR_H
#define ACTOR_H

#include "Mailbox.h"
#include "Component.h"
#include <atomic>
#include <memory>

class ActorScheduler;  // forward decl

class IActor
{
public:
    IActor(const std::string& id, std::unique_ptr<IMailbox> mailbox, ActorScheduler* scheduler);
    virtual ~IActor() = default;

    // Process one message (called by scheduler worker)
    virtual void onMessage(Message&& msg) = 0;

    // Query: does this actor have pending work?
    bool hasWork() const;

    // Get actor ID
    std::string getId() const;


    // Atomically check if actor is already enqueued; if not, mark as enqueued and return true
    bool enqueuedIfNot();

    void markNotEnqueued();

    // Enqueue a message (convenience; delegates to mailbox + scheduler)
    void enqueueMessage(Message&& msg);

    // Get mailbox (for scheduler and testing)
    IMailbox &getMailbox();
    ActorScheduler *getScheduler() const;
private:
    std::string id_;
    std::unique_ptr<IMailbox> mailbox_;
    ActorScheduler *scheduler_;  // not owned; can be null (no auto-schedule)
    std::atomic_bool enqueued_{false};  // atomic flag: is actor already in run-queue?
};

// Concrete impl: Component-based actor
class ComponentActor : public IActor
{
public:
    ComponentActor(const std::string& id,
                   Component* component,
                   ActorScheduler* scheduler = nullptr,
                   std::vector<std::string> targetActorIds = {});

    ~ComponentActor() = default;

    // IActor impl
    void onMessage(Message&& msg) override;

    // Access component (for testing/debugging)
    Component *getComponent() const { return component_; }

private:
    Component *component_{nullptr};  // not owned
    std::vector<std::string> targetActorIds_;  // IDs of actors to send output messages to
};

#endif // ACTOR_H
