#ifndef GRAPHEXECUTIONSANDBOXSIM_H
#define GRAPHEXECUTIONSANDBOXSIM_H

#include <vector>
#include <string>
#include <unordered_map>

enum GraphItemType
{
    Component_Type,
    Connection_Type
};

class Component
{
public:
    Component(const std::string &id, const std::string &type);
    virtual void execute() = 0;
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
    void execute() override;
private:
    int m_tickCount{};
};

class PrinterComponent : public Component
{
public:
    PrinterComponent(const std::string &id);
    void execute() override;
};

std::string new_id(GraphItemType type);

class GraphExecutionSandboxSim
{
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

    void execute();
private:
    std::string m_startupComponentId{};
    std::unordered_map<std::string, std::string> m_startupComponentProperties{};

    std::vector<Component *> m_components;
    std::vector<Connection *> m_connections;
    std::unordered_map<std::string, int> m_pendingComponents;
private:
    void prepareExecution();
    void executeComponent(Component *component);
    void commitState();
};

#endif // GRAPHEXECUTIONSANDBOXSIM_H
