#ifndef ACTOR_H
#define ACTOR_H

#include "Message.h"

class ActorSystem;
class Component;
class Mailbox;
class ActorTaskResult;
class Actor
{
public:
    Actor(ActorSystem *system, const Component *component);
    ~Actor() = default;

    bool hasNextMessage() const;
    ActorTaskResult ProcessNextMessage();
    void enqueueMessage(Message&& message);

    std::string getActorId() const;
private:
    ActorSystem *m_system{nullptr};
    Component *m_component{nullptr};
    Mailbox *m_mailbox{nullptr};
};

#endif // ACTOR_H
