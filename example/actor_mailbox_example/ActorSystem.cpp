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
    for (const auto& actor : actors)
    {
        if (actor->hasMessages())
        {
            readyActors.push(actor);
        }
    }

    if (!readyActors.empty())
    {
        auto nextActor = readyActors.front();
        readyActors.pop();
        return nextActor.get();
    }

    return nullptr; // No ready actors
}

void ActorSystem::send(const std::string & actorId, Message * message)
{
    auto it = std::find(actorIds.begin(), actorIds.end(), actorId);

    if (it != actorIds.end())
    {
        size_t index = std::distance(actorIds.begin(), it);
        actors[index]->enqueueMessage(*message);
    }
}