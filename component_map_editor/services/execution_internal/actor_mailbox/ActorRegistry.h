#ifndef CME_ACTORREGISTRY_H
#define CME_ACTORREGISTRY_H

#include <QMap>
#include <QString>

#include <memory>
#include <mutex>
#include <vector>

namespace cme::actor
{

class IActor;

class ActorRegistry
{
public:
    void registerActor(const QString &id, std::shared_ptr<IActor> actor);
    void deregisterActor(const QString &id);

    std::shared_ptr<IActor> actor(const QString &id) const;
    std::vector<std::shared_ptr<IActor>> allActors() const;
    size_t size() const;

private:
    mutable std::mutex m_mutex;
    QMap<QString, std::shared_ptr<IActor>> m_actors;
};

} // namespace cme::actor

#endif // CME_ACTORREGISTRY_H
