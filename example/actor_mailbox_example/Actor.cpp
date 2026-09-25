#include "Actor.h"

#include "Mailbox.h"

Actor::Actor(ActorSystem *system, const Component *component)
    : m_system(system)
    , m_mailbox(new Mailbox()) {}

void Actor::ProcessNextMessage()
{
    if (m_mailbox->hasMessages())
    {
        Message message = m_mailbox->nextMessage();
        // Process the message here
        // For example, you can call the component's execute method with the message payload
    }
}

void Actor::enqueueMessage(const Message &message)
{
    m_mailbox->enqueueMessage(message);
}

bool Actor::hasMessages() const
{
    return m_mailbox->hasMessages();
}