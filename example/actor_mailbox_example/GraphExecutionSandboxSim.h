#ifndef GRAPHEXECUTIONSANDBOXSIM_H
#define GRAPHEXECUTIONSANDBOXSIM_H

#include <QThreadPool>
#include <deque>

#include "ExecutionStateCapture.h"
#include "ActorScheduler.h"
#include "Component.h"

// Orchestrator-level tracking (separate from actors):
class ExecutionOrchestrator
{
    std::unordered_map<std::string, int> pendingInDegree_;  // Component → in-degree count
    std::deque<std::string> readyQueue_;                     // Components with pending == 0

    void onExecutionComplete(const std::string& componentId)
    {
        // for (auto& target : targets)
        // {
        //     pendingInDegree_[target]--;

        //     if (pendingInDegree_[target] == 0)
        //     {
        //         readyQueue_.push_back(target);
        //         // Trigger initial message to target actor
        //     }
        // }
    }
};

class ActorSystem;
class GraphExecutionSandboxSim: public QObject
{
    Q_OBJECT
    Q_PROPERTY(ExecutionStatus executionStatus READ getExecutionStatus NOTIFY executionStatusChanged)
    Q_PROPERTY(QStringList timeLine READ getTimeline NOTIFY timelineChanged)
public:
    enum class ExecutionStatus
    {
        NOT_STARTED,
        RUNNING,
        PAUSED,
        STEPPING,
        COMPLETED,
        ERROR,
    };
    Q_ENUM(ExecutionStatus)

    GraphExecutionSandboxSim();

    std::vector<Component *> getComponents() const;
    std::vector<Connection *> getConnections() const;

    Component *getComponentById(const QString &id) const;
    Connection *getConnectionById(const QString &id) const;

    std::vector<Connection *> getIncomingConnections(const QString &componentId) const;
    std::vector<Connection *> getOutgoingConnections(const QString &componentId) const;

    void addComponent(Component *component);
    void removeComponent(const QString &id);

    void addConnection(Connection *connection);
    void removeConnection(const QString &id);
    bool configureStartupComponent(const QString &componentId,
                                   const QVariantMap &properties = {});


    void clear();
    Q_INVOKABLE void start();
    Q_INVOKABLE void step();
    Q_INVOKABLE void run();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void reset();


    ExecutionStatus getExecutionStatus() const;
    QStringList getTimeline() const;
signals:
    void executionStatusChanged();
    void timelineChanged();
private:

    std::vector<Component *> m_components;
    std::vector<Connection *> m_connections;
    ActorScheduler *m_scheduler{nullptr};
    ExecutionStatus m_executionStatus{ExecutionStatus::NOT_STARTED};
    QStringList m_timeline;

    QString m_startupComponentId;
    QVariantMap m_startupProperties;

    QString m_lastStepComponentId{};
private:
    bool captureState();
    QVariantMap componentSnapshot(const QString& componentId) const;
    void setExecutionStatus(ExecutionStatus status);

    void onStepCompleted(const ExecutionContext& ctx, const ExecuteResult& result);


};

#endif // GRAPHEXECUTIONSANDBOXSIM_H
