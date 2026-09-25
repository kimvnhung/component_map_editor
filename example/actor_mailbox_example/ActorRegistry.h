#ifndef ACTORREGISTRY_H
#define ACTORREGISTRY_H

#include <QString>
#include <QMap>

#include <mutex>
#include <memory>
#include <vector>

class IActor;
class ActorRegistry
{
public:
    void registerActor(const QString& id, std::shared_ptr<IActor> actor);
    void deregisterActor(const QString& id);

    std::shared_ptr<IActor> getActor(const QString& id) const;
    std::vector<std::shared_ptr<IActor>> getAllActors() const;

private:
    mutable std::mutex mu_;
    QMap<QString, std::shared_ptr<IActor>> actors_;
};

#endif // ACTORREGISTRY_H
