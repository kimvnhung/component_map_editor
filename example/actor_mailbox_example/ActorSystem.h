#ifndef ACTORSYSTEM_H
#define ACTORSYSTEM_H

#include <memory>
#include <vector>
#include <string>
#include <queue>

class Component;
class Message;
class Actor;
class ActorSystem
{
public:
    ActorSystem();
    bool registerActor(const Component* component);
    void unregisterActor(const std::string actorId);

    Actor *nextReadyActor();

    void send(const std::string& actorId, Message* message);
private:
    std::vector<std::shared_ptr<Actor>> actors;
    std::queue<std::shared_ptr<Actor>> readyActors;
    std::vector<std::string> actorIds;
};

#endif // ACTORSYSTEM_H
