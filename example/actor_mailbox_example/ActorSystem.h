#ifndef ACTORSYSTEM_H
#define ACTORSYSTEM_H

#include <memory>
#include <vector>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>

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

    void send(const std::string& actorId, Message&& message);
    void stop();
private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stopped = false;

    std::vector<std::shared_ptr<Actor>> actors;
    std::queue<std::shared_ptr<Actor>> readyActors;
    std::vector<std::string> readyActorIds;
    std::vector<std::string> actorIds;
private:
    void enqueReadyActor(std::shared_ptr<Actor> actor);
};

#endif // ACTORSYSTEM_H
