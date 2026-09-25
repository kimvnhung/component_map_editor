#include "Mailbox.h"

bool Mailbox::hasNextMessage() const
{
    return !m_messages.empty();
}

bool Mailbox::nextMessage(Message& message)
{
    if (!m_messages.empty())
    {
        message = std::move(m_messages.front());
        m_messages.pop();
        return true;
    }

    return false;
}

void Mailbox::enqueueMessage(Message&& msg)
{
    m_messages.push(std::move(msg));
}