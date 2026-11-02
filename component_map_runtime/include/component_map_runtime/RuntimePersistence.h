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

struct PersistedInboxEvent
{
    RuntimeEvent event;
    QString status;
    QByteArray requestFingerprint;
    RuntimeFailure failure;
    qint64 createdAt = 0;
};

struct DeploymentSnapshot
{
    QString deploymentId;
    QString rootGraphId;
    QString rootGraphVersion;
    QString dependencyManifest;
    QString definitionHashes;
    QString executionPlanHash;
    QByteArray serializedDefinitions;
};

struct PersistedComponentState
{
    QString deploymentId;
    QString qualifiedPath;
    QString componentType;
    QByteArray state;
    int schemaVersion = 0;
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
    bool saveDeploymentSnapshot(const DeploymentSnapshot &snapshot, QString *error = nullptr);
    bool loadDeploymentSnapshot(const QString &deploymentId, DeploymentSnapshot *snapshot,
                               QString *error = nullptr) const;
    bool saveComponentState(const QString &instanceId,
                            const QString &componentType,
                            const QByteArray &state,
                            int schemaVersion,
                            QString *error = nullptr);
    QHash<QString, QByteArray> loadComponentStates(QString *error = nullptr) const;
    bool saveComponentState(const QString &deploymentId, const QString &qualifiedPath,
                            const QString &componentType, const QByteArray &state,
                            int schemaVersion, QString *error = nullptr);
    QList<PersistedComponentState> loadComponentStateRecords(QString *error = nullptr) const;

    bool recordAcceptedEvent(const RuntimeEvent &event,
                             const QByteArray &requestFingerprint = {},
                             QString *error = nullptr);
    bool recordAcceptedEvent(const RuntimeEvent &event, QString *error)
    {
        return recordAcceptedEvent(event, QByteArray(), error);
    }
    QList<PersistedInboxEvent> loadInboxEvents(QString *error = nullptr) const;
    QList<PersistedInboxEvent> loadIncompleteEvents(QString *error = nullptr) const;
    bool markEventProcessing(const QString &eventId, QString *error = nullptr);
    bool recordEventCompletion(const QString &eventId, bool succeeded,
                               const RuntimeFailure &failure = {}, QString *error = nullptr);

    bool beginTransaction(QString *error = nullptr);
    bool commitTransaction(QString *error = nullptr);
    bool rollbackTransaction(QString *error = nullptr);
    bool saveEventResult(const QString &eventId, const QString &deploymentId,
                         const QString &qualifiedPath, const QString &componentType,
                         const QByteArray &state, int schemaVersion, bool succeeded,
                         const RuntimeFailure &failure = {}, QString *error = nullptr);
    bool markRuntimeStatus(const QString &status, QString *error = nullptr);
    QString runtimeStatus(QString *error = nullptr) const;
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
