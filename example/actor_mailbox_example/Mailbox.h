#ifndef MAILBOX_H
#define MAILBOX_H

#include <queue>
#include "Message.h"

class Mailbox
{
public:
    bool hasMessages() const;
    Message nextMessage() const;

    void enqueueMessage(const Message &message);
private:
    std::queue<Message> m_messages;
};

#endif // MAILBOX_H
