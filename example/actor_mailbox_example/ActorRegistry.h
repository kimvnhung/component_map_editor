#ifndef ACTORREGISTRY_H
#define ACTORREGISTRY_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>

class IActor;
class ActorRegistry
{
public:
    void registerActor(const std::string& id, std::shared_ptr<IActor> actor);
    void deregisterActor(const std::string& id);

    std::shared_ptr<IActor> getActor(const std::string& id) const;
    std::vector<std::shared_ptr<IActor>> getAllActors() const;

private:
    mutable std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<IActor>> actors_;
};

#endif // ACTORREGISTRY_H
