#include "ActorRegistry.h"

void ActorRegistry::registerActor(const QString& id, std::shared_ptr<IActor> actor)
{
    std::lock_guard<std::mutex> lock(mu_);
    actors_[id] = actor;
}

void ActorRegistry::deregisterActor(const QString& id)
{
    std::lock_guard<std::mutex> lock(mu_);
    actors_.remove(id);
}

std::shared_ptr<IActor> ActorRegistry::getActor(const QString& id) const
{
    std::lock_guard<std::mutex> lock(mu_);
    return actors_.value(id, nullptr);
}

std::vector<std::shared_ptr<IActor>> ActorRegistry::getAllActors() const
{
    std::lock_guard<std::mutex> lock(mu_);
    std::vector<std::shared_ptr<IActor>> allActors;

    for (const auto& actor : actors_.values())
    {
        allActors.push_back(actor);
    }

    return allActors;
}