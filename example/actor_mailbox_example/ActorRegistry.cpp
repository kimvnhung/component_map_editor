#include "ActorRegistry.h"

#include "Actor.h"

void ActorRegistry::registerActor(const std::string& id, std::shared_ptr<IActor> actor)
{
    std::lock_guard<std::mutex> lock(mu_);
    actors_[id] = actor;
}

void ActorRegistry::deregisterActor(const std::string& id)
{
    std::lock_guard<std::mutex> lock(mu_);
    actors_.erase(id);
}

std::shared_ptr<IActor> ActorRegistry::getActor(const std::string& id) const
{
    std::lock_guard<std::mutex> lock(mu_);
    auto it = actors_.find(id);
    return (it != actors_.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<IActor>> ActorRegistry::getAllActors() const
{
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<std::shared_ptr<IActor>> allActors;

    for (const auto& pair : actors_)
    {
        allActors.push_back(pair.second);
    }

    return allActors;
}