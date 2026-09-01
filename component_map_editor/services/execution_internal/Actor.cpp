#include "Actor.h"

IActor::IActor(const QString& id, std::unique_ptr<IMailbox> mailbox, ActorScheduler* scheduler)
    : id_(id), mailbox_(std::move(mailbox)), scheduler_(scheduler), stateHistory_(std::make_unique<ActorStateHistory>())
{
}

bool IActor::hasWork() const
{
    return mailbox_->size() > 0;
}

QString IActor::getId() const
{
    return id_;
}

bool IActor::enqueuedIfNot()
{
    bool expected = false;
    return enqueued_.compare_exchange_strong(expected, true);
}

void IActor::markNotEnqueued()
{
    enqueued_ = false;
}

void IActor::enqueueMessage(Message&& msg)
{
    if (mailbox_->enqueue(std::move(msg)) && scheduler_)
    {
        // scheduler_->enqueueActorWork(this);
    }
}

IMailbox &IActor::getMailbox()
{
    return *mailbox_;
}

ActorScheduler *IActor::getScheduler() const
{
    return scheduler_;
}

ActorStateHistory *IActor::stateHistory() const
{
    return stateHistory_.get();
}

ComponentActor::ComponentActor(const QString& id,
                               IExecutionSematicsProvider* component,
                               ActorScheduler* scheduler,
                               QMap<QString, QStringList> connectionRoutingTable)
    : IActor(id, std::make_unique<Mailbox>(), scheduler)
    , component_(component)
{
    Q_UNUSED(connectionRoutingTable);
}

void ComponentActor::onMessage(Message&& msg)
{
    if (!component_)
    {
        return;
    }

    // TODO: Implement the logic to process the message using the component's execution semantics.
}
