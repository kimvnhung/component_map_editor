#ifndef ACTOR_H
#define ACTOR_H

#include "Mailbox.h"
#include "Message.h"

class ActorSystem;
class Actor
{
public:
    Actor(ActorSystem *system);
    virtual ~Actor() = default;

    virtual void ProcessNextMessage() = 0;
    void send(const Message &message, Actor *recipient);

    Mailbox mailbox;
private:
    ActorSystem *m_system{nullptr};
};

#endif // ACTOR_H
