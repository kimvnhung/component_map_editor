#include "Actor.h"

#include "Mailbox.h"
#include "ActorScheduler.h"
#include "GraphExecutionSandboxSim.h"

Actor::Actor(ActorSystem *system, const Component *component)
    : m_system(system)
    , m_component(const_cast<Component *>(component))
    , m_mailbox(new Mailbox()) {}

ActorTaskResult Actor::ProcessNextMessage()
{
    if (m_mailbox->hasMessages())
    {
        Message message = m_mailbox->nextMessage();
        // Process the message here
        // For example, you can call the component's execute method with the message payload
        Tokens outputTokens;
        bool result = m_component->execute(outputTokens, message.getTokens(), message.getComponentSnapshot());
        Message responseMessage;
        responseMessage.setActorId(m_component->getId());
        responseMessage.setTokens(outputTokens);
        ActorTaskResult taskResult;
        taskResult.status = result ? ActorTaskResult::Status::Success : ActorTaskResult::Status::Failure;
        taskResult.outputMessage = new Message(responseMessage);
        return taskResult;
    }

    return ActorTaskResult{ActorTaskResult::Status::Failure, nullptr, "No messages to process."};
}

void Actor::enqueueMessage(const Message &message)
{
    m_mailbox->enqueueMessage(message);
}

bool Actor::hasMessages() const
{
    return m_mailbox->hasMessages();
}