#include "Actor.h"

#include <base_log.h>

#include "ActorScheduler.h"

IActor::IActor(const std::string& id, std::unique_ptr<IMailbox> mailbox, ActorScheduler* scheduler)
    : id_(id)
    , mailbox_(std::move(mailbox))
    , scheduler_(scheduler)
{
}

bool IActor::hasWork() const
{
    return !mailbox_->empty();
}

std::string IActor::getId() const
{
    return id_;
}

bool IActor::enqueuedIfNot()
{
    return !enqueued_.exchange(true);
}

void IActor::markNotEnqueued()
{
    enqueued_.store(false);
}

void IActor::enqueueMessage(Message&& msg)
{
    bool wasEmpty = mailbox_->empty();// check trước

    if (mailbox_->enqueue(std::move(msg)))
    {
        if (wasEmpty)
        {
            // If the mailbox was empty before this enqueue, we need to schedule the actor for processing
            // Use atomic exchange to avoid race conditions
            // old = value
            // value = new
            // return old
            if (enqueuedIfNot())
            {
                // Actor was not already enqueued; schedule it
                if (scheduler_)
                {
                    scheduler_->enqueueActorWork(this);
                }
            }
        }
    }
    else
    {
        // Handle backpressure: message was dropped or rejected
        LOGWF("[ComponentActor][{}] Mailbox full. Message dropped.", id_);
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

ComponentActor::ComponentActor(const std::string & id,
                               Component * component,
                               ActorScheduler * scheduler,
                               std::vector<std::string> targetActorIds)
    : IActor(id, std::make_unique<MailboxImpl>(1024, BackpressurePolicy::DROP_NEWEST), scheduler)
    , component_(component)
    , targetActorIds_(std::move(targetActorIds))
{
}

void ComponentActor::onMessage(Message && msg)
{
    // Process the message using the component's logic
    if (component_)
    {
        Tokens output;
        bool res = component_->execute(output, msg.tokens, msg.componentSnapshot);

        if (res)
        {
            LOGDF("[ComponentActor][{}] Component executed successfully for message ID {}. Output tokens: {}",
                  getId(), msg.id, token2string(output));

            // Route output tokens to target actors
            for (const std::string& targetId : targetActorIds_)
            {
                Message outputMsg{msg.id, targetId, output, component_->snapshot()};

                if (getScheduler())
                {
                    getScheduler()->routeMessage(targetId, std::move(outputMsg));
                }
                else
                {
                    LOGWF("[ComponentActor][{}] No scheduler available to route message to {}.", getId(), targetId);
                }
            }
        }
        else
        {
            LOGWF("[ComponentActor][{}] Component execution failed for message ID {}.", getId(), msg.id);
        }
    }
    else
    {
        LOGWF("[ComponentActor][{}] No component associated with this actor.", getId());
    }
}

