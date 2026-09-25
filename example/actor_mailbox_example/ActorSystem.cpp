#include "ActorSystem.h"

#include "Actor.h"
#include "GraphExecutionSandboxSim.h"

ActorSystem::ActorSystem() {}

bool ActorSystem::registerActor(const Component* component)
{
    if (!component)
    {
        return false;
    }

    std::string actorId = component->getId();

    // Check if the actor is already registered
    if (std::find(actorIds.begin(), actorIds.end(), actorId) != actorIds.end())
    {
        return false; // Actor already registered
    }

    actors.push_back(std::make_shared<Actor>(this, component));
    actorIds.push_back(actorId);
    return true;
}

void ActorSystem::unregisterActor(const std::string actorId)
{
    auto it = std::find(actorIds.begin(), actorIds.end(), actorId);

    if (it != actorIds.end())
    {
        size_t index = std::distance(actorIds.begin(), it);
        actors.erase(actors.begin() + index);
        actorIds.erase(it);
    }
}

Actor *ActorSystem::nextReadyActor()
{
    std::unique_lock lock(m_mutex);

    m_cv.wait(lock, [this]
    {
        return m_stopped || !readyActors.empty();
    });

    if (m_stopped)
    {
        return nullptr;
    }

    Actor* actor = readyActors.front().get();
    readyActors.pop();

    return actor;

    return nullptr; // No ready actors
}
void ActorSystem::stop()
{
    {
        std::lock_guard lock(m_mutex);
        m_stopped = true;
    }

    m_cv.notify_all();
}

void ActorSystem::enqueReadyActor(std::shared_ptr<Actor> actor)
{
    {
        std::unique_lock lock(m_mutex);
        readyActors.push(actor);
    }
    m_cv.notify_one();
}

void ActorSystem::send(const std::string & actorId, Message * message)
{
    auto it = std::find(actorIds.begin(), actorIds.end(), actorId);

    if (it != actorIds.end())
    {
        size_t index = std::distance(actorIds.begin(), it);
        actors[index]->enqueueMessage(*message);
        enqueReadyActor(actors[index]);
    }
}