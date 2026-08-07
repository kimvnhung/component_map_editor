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
    ActorScheduler *m_scheduler{nullptr};
    bool m_isRunning{false};
    QStringList m_timeline;

    QString m_startupComponentId;
    QVariantMap m_startupProperties;
private:
    bool captureState();
    void setRunning(bool running);
};

#endif // GRAPHEXECUTIONSANDBOXSIM_H
