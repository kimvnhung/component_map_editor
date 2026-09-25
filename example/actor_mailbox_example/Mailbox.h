#ifndef MAILBOX_H
#define MAILBOX_H

#include <queue>
#include "Message.h"

class Mailbox
{
public:
    bool hasNextMessage() const;
    bool nextMessage(Message& message) ;
    void enqueueMessage(Message&& msg);
private:
    std::queue<Message> m_messages;
};

#endif // MAILBOX_H
