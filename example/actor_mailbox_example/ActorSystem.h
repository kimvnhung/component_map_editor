#ifndef ACTORSYSTEM_H
#define ACTORSYSTEM_H

#include "

class Actor;
class ActorSystem
{
public:
    ActorSystem();
private:
    std::vector<std::shared_ptr<Actor>> actors;
};

#endif // ACTORSYSTEM_H
