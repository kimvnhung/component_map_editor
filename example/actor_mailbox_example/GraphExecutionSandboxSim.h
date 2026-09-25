#ifndef GRAPHEXECUTIONSANDBOXSIM_H
#define GRAPHEXECUTIONSANDBOXSIM_H

#include <string>
#include <unordered_map>
#include <QThreadPool>
#include "ActorScheduler.h"
#include "Component.h"


class ActorSystem;
class GraphExecutionSandboxSim: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(QStringList timeLine READ getTimeline NOTIFY timelineChanged)
public:
    GraphExecutionSandboxSim();

    bool isValid() const;

    std::vector<Component *> getComponents() const;
    std::vector<Connection *> getConnections() const;

    Component *getComponentById(const std::string &id) const;
    Connection *getConnectionById(const std::string &id) const;

    std::vector<Connection *> getIncomingConnections(const std::string &componentId) const;
    std::vector<Connection *> getOutgoingConnections(const std::string &componentId) const;

    void addComponent(Component *component);
    void removeComponent(const std::string &id);

    void addConnection(Connection *connection);
    void removeConnection(const std::string &id);
    bool configureStartupComponent(const std::string &componentId,
                                   const std::unordered_map<std::string, std::string> &properties = {});

    void clear();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void execute();
    bool isRunning() const;
    QStringList getTimeline() const;
signals:
    void isRunningChanged();
    void timelineChanged();
private:

    std::vector<Component *> m_components;
    std::vector<Connection *> m_connections;
    ActorSystem *m_actorSystem{nullptr};
    ActorScheduler *m_scheduler{nullptr};
    bool m_isRunning{false};
    QStringList m_timeline;
private:
    bool captureState();
    void prepareExecution();
    void executeComponent(Component *component);
    void commitState();
    void setRunning(bool running);
    void routeTokens(const ActorTaskResult& result);
};

#endif // GRAPHEXECUTIONSANDBOXSIM_H
