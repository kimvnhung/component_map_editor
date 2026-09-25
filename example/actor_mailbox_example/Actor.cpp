#include "Actor.h"

#include <base_log.h>

#include "ActorScheduler.h"
#include "ExecutionStateCapture.h"

IActor::IActor(const QString& id, std::unique_ptr<IMailbox> mailbox, ActorScheduler* scheduler)
    : id_(id)
    , mailbox_(std::move(mailbox))
    , scheduler_(scheduler)
{
}

bool IActor::hasWork() const
{
    return !mailbox_->empty();
}

QString IActor::getId() const
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
        LOGWF("[ComponentActor][{}] Mailbox full. Message dropped.", id_.toStdString());
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

ComponentActor::ComponentActor(const QString& id,
                               Component * component,
                               ActorScheduler * scheduler,
                               QMap<QString, QStringList> connectionRoutingTable)
    : IActor(id, std::make_unique<MailboxImpl>(1024, BackpressurePolicy::DROP_NEWEST), scheduler)
    , component_(component)
{
}

void ComponentActor::onMessage(Message && msg)
{
    ExecuteResult result;
    ExecutionContext ctx
    {
        msg.sourceId,
        getId(),
        msg.tokens,
        component_ == nullptr ? component_->snapshot() : QVariantMap()
    };

    // Process the message using the component's logic
    if (component_)
    {
        bool res = component_->execute(result.outputState, ctx.inputTokens, ctx.componentSnapshot);
        result.success = res;

        if (res)
        {
            LOGDF("[ComponentActor][{}] Component executed successfully for message ID {}. Output tokens: {}",
                  getId().toStdString(), msg.id, token2string(result.outputState).toStdString());

            result.message = "Component executed successfully.";
        }
        else
        {
            result.message = "Component execution failed.";
            LOGWF("[ComponentActor][{}] Component execution failed for message ID {}.", getId().toStdString(), msg.id);
        }
    }
    else
    {
        LOGWF("[ComponentActor][{}] No component associated with this actor.", getId().toStdString());
        result.success = false;
        result.message = "No component associated with this actor.";
    }

    // Route output tokens to connected components
    auto scheduler = getScheduler();

    if (scheduler)
    {
        scheduler->handleActorProcessFinished(ctx, result);
    }
    else
    {
        LOGWF("[ComponentActor][{}] Scheduler not available for routing tokens.", getId().toStdString());
    }
}

