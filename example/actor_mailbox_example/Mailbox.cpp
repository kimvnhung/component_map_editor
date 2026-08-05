#include "Mailbox.h"

bool Mailbox::hasMessages() const
{
    return !m_messages.empty();
}

Message Mailbox::nextMessage()
{
    if (m_messages.empty())
    {
        return Message();
    }

    Message message = m_messages.front();
    m_messages.pop();
    return message;
}

void Mailbox::enqueueMessage(const Message &message)
{
    m_messages.push(message);
}