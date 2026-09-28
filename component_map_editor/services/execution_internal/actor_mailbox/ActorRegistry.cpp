#include "ActorRegistry.h"

#include "Actor.h"

namespace cme::actor
{

void ActorRegistry::registerActor(const QString &id, std::shared_ptr<IActor> actor)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_actors.insert(id, std::move(actor));
}

void ActorRegistry::deregisterActor(const QString &id)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_actors.remove(id);
}

std::shared_ptr<IActor> ActorRegistry::actor(const QString &id) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_actors.value(id, nullptr);
}

std::vector<std::shared_ptr<IActor>> ActorRegistry::allActors() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<IActor>> result;
    result.reserve(m_actors.size());

    for (const auto &actor : m_actors)
    {
        result.push_back(actor);
    }

    return result;
}

size_t ActorRegistry::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<size_t>(m_actors.size());
}

} // namespace cme::actor
