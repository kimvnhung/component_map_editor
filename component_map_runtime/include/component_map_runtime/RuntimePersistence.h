#ifndef COMPONENT_MAP_RUNTIME_RUNTIME_PERSISTENCE_H
#define COMPONENT_MAP_RUNTIME_RUNTIME_PERSISTENCE_H

#include "component_map_runtime/RuntimeCore.h"

#include <QHash>
#include <QString>
#include <deployment.pb.h>
#include <memory>

struct sqlite3;

namespace cme::runtime
{

struct PersistedEventHistory
{
    QString eventId;
    QString status;
    QString failureCode;
    QString failureMessage;
    qint64 createdAt = 0;
};

struct PersistedRuntimeLog
{
    QString level;
    QString code;
    QString message;
    qint64 createdAt = 0;
};

class RuntimePersistence
{
public:
    RuntimePersistence() = default;
    ~RuntimePersistence();

    RuntimePersistence(const RuntimePersistence &) = delete;
    RuntimePersistence &operator=(const RuntimePersistence &) = delete;

    bool open(const QString &databasePath, const QString &runtimeId, QString *error = nullptr);
    void close();
    bool isOpen() const;
    QString databasePath() const;

    bool saveGraph(const cme::runtime::v1::DeploymentGraph &graph, QString *error = nullptr);
    bool saveComponentState(const QString &instanceId,
                            const QString &componentType,
                            const QByteArray &state,
                            int schemaVersion,
                            QString *error = nullptr);
    QHash<QString, QByteArray> loadComponentStates(QString *error = nullptr) const;

    bool recordAcceptedEvent(const RuntimeEvent &event, QString *error = nullptr);
    bool recordEventCompletion(const QString &eventId, bool succeeded,
                               const RuntimeFailure &failure = {}, QString *error = nullptr);
    bool recordRejection(const RejectionRecord &record, QString *error = nullptr);
    bool recordFailure(const FailureRecord &record, QString *error = nullptr);
    bool recordLog(const QString &level, const QString &code,
                   const QString &message, QString *error = nullptr);

    QList<PersistedEventHistory> eventHistory(int limit = 100, QString *error = nullptr) const;
    QList<PersistedRuntimeLog> runtimeLogs(int limit = 100, QString *error = nullptr) const;
    int eventHistoryCount(QString *error = nullptr) const;
    int rejectionCount(QString *error = nullptr) const;
    int logCount(QString *error = nullptr) const;
    int pendingEventCount(QString *error = nullptr) const;
    bool resetExecutionState(QString *error = nullptr);

private:
    bool execute(const QString &sql, QString *error = nullptr) const;
    bool migrate(QString *error);
    bool setError(const QString &message, QString *error) const;
    QString sqliteError() const;

    sqlite3 *m_database = nullptr;
    QString m_databasePath;
    QString m_runtimeId;
};

} // namespace cme::runtime

#endif // COMPONENT_MAP_RUNTIME_RUNTIME_PERSISTENCE_H
