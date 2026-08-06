#include "Actor.h"

#include "Mailbox.h"
#include "ActorScheduler.h"
#include "GraphExecutionSandboxSim.h"

Actor::Actor(ActorSystem *system, const Component *component)
    : m_system(system)
    , m_component(const_cast<Component *>(component))
    , m_mailbox(new Mailbox()) {}

bool Actor::hasNextMessage() const
{
    return m_mailbox->hasNextMessage();
}

ActorTaskResult Actor::ProcessNextMessage()
{
    Message message;

    if (m_mailbox->nextMessage(message))
    {
        // Process the message here
        // For example, you can call the component's execute method with the message payload
        Tokens outputTokens;
        bool result = m_component->execute(outputTokens, message.getTokens(), message.getComponentSnapshot());
        ActorTaskResult taskResult;
        taskResult.status = result ? ActorTaskResult::Status::Success : ActorTaskResult::Status::Failure;
        taskResult.outputMessage.setActorId(m_component->getId());
        taskResult.outputMessage.setTokens(outputTokens);
        return taskResult;
    }

    return ActorTaskResult{ActorTaskResult::Status::Failure, {}, "No messages to process."};
}

std::string Actor::getActorId() const
{
    return m_component->getId();
}

void Actor::enqueueMessage(Message&& message)
{
    m_mailbox->enqueueMessage(std::move(message));
}