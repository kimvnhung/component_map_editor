#ifndef GRAPHEXECUTIONSANDBOXSIM_H
#define GRAPHEXECUTIONSANDBOXSIM_H

#include <vector>
#include <string>
#include <unordered_map>
#include <QThreadPool>
#include "ActorScheduler.h"

using Tokens = std::unordered_map<std::string, std::string>;

enum GraphItemType
{
    Component_Type,
    Connection_Type
};

class Component
{
public:
    Component(const std::string &id, const std::string &type);
    virtual void execute(const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}, const Tokens &outputTokens = {})
        = 0;
    std::string getId() const;
    std::string getType() const;

    void addProperty(const std::string &key, const std::string &value);
    void removeProperty(const std::string &key);
    std::string getProperty(const std::string &key) const;
private:
    std::string m_id{};
    std::string m_type{};
    std::unordered_map<std::string, std::string> m_properties{};
};

class Connection
{
public:
    Connection(const std::string &id, const std::string &sourceComponentId, const std::string &targetComponentId);
    std::string getId() const;
    std::string getSourceComponentId() const;
    std::string getTargetComponentId() const;

    void setSourceComponentId(const std::string &sourceComponentId);
    void setTargetComponentId(const std::string &targetComponentId);
    void addProperty(const std::string &key, const std::string &value);
    void removeProperty(const std::string &key);
    std::string getProperty(const std::string &key) const;
private:
    std::string m_id{};
    std::string m_sourceComponentId{};
    std::string m_targetComponentId{};
    std::unordered_map<std::string, std::string> m_properties{};
};

class ClockComponent : public Component
{
public:
    ClockComponent(const std::string &id);
    void execute(const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}, const Tokens &outputTokens = {})
    override;
private:
    int m_tickCount{};
};

class PrinterComponent : public Component
{
public:
    PrinterComponent(const std::string &id);
    void execute(const Tokens &inputTokens = {}, const Tokens &componentSnapshot = {}, const Tokens &outputTokens = {})
    override;
};

std::string new_id(GraphItemType type);

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

    void routeTokens(const ActorTaskResult& result);
};

#endif // GRAPHEXECUTIONSANDBOXSIM_H
