#include "component_map_runtime/RuntimePersistence.h"

#include <QDir>
#include <QFileInfo>
#include <sqlite3.h>

namespace
{
using namespace cme::runtime;

bool bindText(sqlite3_stmt *statement, int index, const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    return sqlite3_bind_text(statement, index, utf8.constData(), utf8.size(), SQLITE_TRANSIENT)
           == SQLITE_OK;
}

bool bindBytes(sqlite3_stmt *statement, int index, const QByteArray &value)
{
    return sqlite3_bind_blob(statement, index, value.constData(), value.size(), SQLITE_TRANSIENT)
           == SQLITE_OK;
}

QString columnText(sqlite3_stmt *statement, int column)
{
    const auto *text = sqlite3_column_text(statement, column);
    return text ? QString::fromUtf8(reinterpret_cast<const char *>(text)) : QString();
}

QString discoveryKindName(cme::runtime::DiscoveryEvent::Kind kind)
{
    return cme::runtime::discoveryEventKindName(kind);
}

QString discoveryStatusName(cme::runtime::ComponentStatus status)
{
    switch (status)
    {
    case cme::runtime::ComponentStatus::Registered: return QStringLiteral("registered");
    case cme::runtime::ComponentStatus::Available: return QStringLiteral("available");
    case cme::runtime::ComponentStatus::Unavailable: return QStringLiteral("unavailable");
    case cme::runtime::ComponentStatus::Rejected: return QStringLiteral("rejected");
    case cme::runtime::ComponentStatus::Removed: return QStringLiteral("removed");
    }
    return QStringLiteral("unknown");
}

bool bindNullableText(sqlite3_stmt *statement, int index, const QString &value)
{
    if (value.isEmpty())
        return sqlite3_bind_null(statement, index) == SQLITE_OK;
    return bindText(statement, index, value);
}

QByteArray columnBytes(sqlite3_stmt *statement, int column)
{
    const auto *bytes = sqlite3_column_blob(statement, column);
    const int size = sqlite3_column_bytes(statement, column);
    return bytes && size > 0 ? QByteArray(static_cast<const char *>(bytes), size) : QByteArray();
}
}

namespace cme::runtime
{

RuntimePersistence::~RuntimePersistence()
{
    close();
}

bool RuntimePersistence::open(const QString &databasePath, const QString &runtimeId, QString *error)
{
    close();
    m_databasePath = databasePath;
    m_runtimeId = runtimeId;
    if (databasePath != QStringLiteral(":memory:"))
    {
        const QFileInfo info(databasePath);
        if (!info.absolutePath().isEmpty() && !QDir().mkpath(info.absolutePath()))
        {
            return setError(QStringLiteral("Unable to create database directory."), error);
        }
    }
    const QByteArray path = databasePath.toUtf8();
    if (sqlite3_open_v2(path.constData(), &m_database,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK)
    {
        const QString message = sqliteError();
        close();
        return setError(message, error);
    }
    if (!execute(QStringLiteral("PRAGMA journal_mode=WAL;"), error)
        || !execute(QStringLiteral("PRAGMA foreign_keys=ON;"), error)
        || !migrate(error))
    {
        close();
        return false;
    }

    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT INTO runtime_instances(runtime_id, status, updated_at) "
                      "VALUES(?, 'stopped', strftime('%s','now')) "
                      "ON CONFLICT(runtime_id) DO UPDATE SET updated_at=excluded.updated_at";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, runtimeId)
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement)
        {
            sqlite3_finalize(statement);
        }
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

void RuntimePersistence::close()
{
    if (m_database)
    {
        sqlite3_close(m_database);
        m_database = nullptr;
    }
}

bool RuntimePersistence::isOpen() const
{
    return m_database != nullptr;
}

QString RuntimePersistence::databasePath() const
{
    return m_databasePath;
}

bool RuntimePersistence::saveGraph(const cme::runtime::v1::DeploymentGraph &graph, QString *error)
{
    if (!m_database)
    {
        return setError(QStringLiteral("Database is not open."), error);
    }
    std::string serialized;
    if (!graph.SerializeToString(&serialized))
    {
        return setError(QStringLiteral("Unable to serialize deployment graph."), error);
    }
    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT INTO graph_versions(graph_version, graph_blob, active, created_at) "
                      "VALUES(?, ?, 0, strftime('%s','now')) "
                      "ON CONFLICT(graph_version) DO UPDATE SET graph_blob=excluded.graph_blob";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, QString::fromStdString(graph.graph_version()))
        || sqlite3_bind_blob(statement, 2, serialized.data(), static_cast<int>(serialized.size()), SQLITE_TRANSIENT) != SQLITE_OK
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

bool RuntimePersistence::saveDeploymentSnapshot(const DeploymentSnapshot &snapshot, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT INTO deployment_snapshots(deployment_id, root_graph_id, root_graph_version, dependency_manifest, definition_hashes, execution_plan_hash, serialized_definitions, created_at) VALUES(?, ?, ?, ?, ?, ?, ?, strftime('%s','now')) ON CONFLICT(deployment_id) DO UPDATE SET root_graph_id=excluded.root_graph_id, root_graph_version=excluded.root_graph_version, dependency_manifest=excluded.dependency_manifest, definition_hashes=excluded.definition_hashes, execution_plan_hash=excluded.execution_plan_hash, serialized_definitions=excluded.serialized_definitions, created_at=excluded.created_at";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, snapshot.deploymentId) || !bindText(statement, 2, snapshot.rootGraphId)
        || !bindText(statement, 3, snapshot.rootGraphVersion) || !bindText(statement, 4, snapshot.dependencyManifest)
        || !bindText(statement, 5, snapshot.definitionHashes) || !bindText(statement, 6, snapshot.executionPlanHash)
        || !bindBytes(statement, 7, snapshot.serializedDefinitions) || sqlite3_step(statement) != SQLITE_DONE) {
        if (statement) sqlite3_finalize(statement); return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement); return true;
}

bool RuntimePersistence::loadDeploymentSnapshot(const QString &deploymentId, DeploymentSnapshot *snapshot, QString *error) const
{
    if (!m_database || !snapshot) return setError(QStringLiteral("Database is not open or snapshot is null."), error);
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database, "SELECT deployment_id, root_graph_id, root_graph_version, dependency_manifest, definition_hashes, execution_plan_hash, serialized_definitions FROM deployment_snapshots WHERE deployment_id=?", -1, &statement, nullptr) != SQLITE_OK || !bindText(statement, 1, deploymentId)) {
        if (statement) sqlite3_finalize(statement); return setError(sqliteError(), error);
    }
    const int result = sqlite3_step(statement);
    if (result == SQLITE_ROW) { snapshot->deploymentId=columnText(statement,0); snapshot->rootGraphId=columnText(statement,1); snapshot->rootGraphVersion=columnText(statement,2); snapshot->dependencyManifest=columnText(statement,3); snapshot->definitionHashes=columnText(statement,4); snapshot->executionPlanHash=columnText(statement,5); snapshot->serializedDefinitions=columnBytes(statement,6); sqlite3_finalize(statement); return true; }
    sqlite3_finalize(statement); return setError(QStringLiteral("Deployment snapshot not found."), error);
}

bool RuntimePersistence::saveComponentState(const QString &deploymentId, const QString &qualifiedPath,
                                            const QString &componentType, const QByteArray &state,
                                            int schemaVersion, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT INTO component_state_v2(deployment_id, qualified_path, component_type, state_blob, schema_version, updated_at) VALUES(?, ?, ?, ?, ?, strftime('%s','now')) ON CONFLICT(deployment_id, qualified_path) DO UPDATE SET component_type=excluded.component_type, state_blob=excluded.state_blob, schema_version=excluded.schema_version, updated_at=excluded.updated_at";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK || !bindText(statement,1,deploymentId) || !bindText(statement,2,qualifiedPath) || !bindText(statement,3,componentType) || !bindBytes(statement,4,state) || sqlite3_bind_int(statement,5,schemaVersion) != SQLITE_OK || sqlite3_step(statement) != SQLITE_DONE) { if (statement) sqlite3_finalize(statement); return setError(sqliteError(),error); }
    sqlite3_finalize(statement); return true;
}

QList<PersistedComponentState> RuntimePersistence::loadComponentStateRecords(QString *error) const
{
    QList<PersistedComponentState> result; if (!m_database) { setError(QStringLiteral("Database is not open."),error); return result; }
    sqlite3_stmt *statement=nullptr; if (sqlite3_prepare_v2(m_database,"SELECT deployment_id, qualified_path, component_type, state_blob, schema_version FROM component_state_v2",-1,&statement,nullptr)!=SQLITE_OK) { setError(sqliteError(),error); return result; }
    while (sqlite3_step(statement)==SQLITE_ROW) result.append({columnText(statement,0),columnText(statement,1),columnText(statement,2),columnBytes(statement,3),sqlite3_column_int(statement,4)});
    sqlite3_finalize(statement); return result;
}

bool RuntimePersistence::saveComponentState(const QString &instanceId,
                                            const QString &componentType,
                                            const QByteArray &state,
                                            int schemaVersion,
                                            QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT INTO component_state(instance_id, component_type, state_blob, "
                      "schema_version, updated_at) VALUES(?, ?, ?, ?, strftime('%s','now')) "
                      "ON CONFLICT(instance_id) DO UPDATE SET component_type=excluded.component_type, "
                      "state_blob=excluded.state_blob, schema_version=excluded.schema_version, "
                      "updated_at=excluded.updated_at";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, instanceId)
        || !bindText(statement, 2, componentType)
        || !bindBytes(statement, 3, state)
        || sqlite3_bind_int(statement, 4, schemaVersion) != SQLITE_OK
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

QHash<QString, QByteArray> RuntimePersistence::loadComponentStates(QString *error) const
{
    QHash<QString, QByteArray> states;
    if (!m_database)
    {
        setError(QStringLiteral("Database is not open."), error);
        return states;
    }
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database, "SELECT instance_id, state_blob FROM component_state",
                           -1, &statement, nullptr) != SQLITE_OK)
    {
        setError(sqliteError(), error);
        return states;
    }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        states.insert(columnText(statement, 0), columnBytes(statement, 1));
    }
    if (sqlite3_errcode(m_database) != SQLITE_OK
        && sqlite3_errcode(m_database) != SQLITE_ROW
        && sqlite3_errcode(m_database) != SQLITE_DONE)
    {
        setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return states;
}

bool RuntimePersistence::recordAcceptedEvent(const RuntimeEvent &event,
                                               const QByteArray &requestFingerprint,
                                               QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);

    // An idempotency key is a durable request identity.  Reusing it with a
    // different fingerprint must never silently turn into a second request.
    if (!event.idempotencyKey.isEmpty())
    {
        sqlite3_stmt *existing = nullptr;
        const char *lookup = "SELECT request_fingerprint FROM event_inbox "
                             "WHERE idempotency_key=? ORDER BY created_at DESC LIMIT 1";
        if (sqlite3_prepare_v2(m_database, lookup, -1, &existing, nullptr) != SQLITE_OK
            || !bindText(existing, 1, event.idempotencyKey))
        {
            if (existing) sqlite3_finalize(existing);
            return setError(sqliteError(), error);
        }
        const int result = sqlite3_step(existing);
        if (result == SQLITE_ROW)
        {
            const QByteArray stored = columnBytes(existing, 0);
            sqlite3_finalize(existing);
            if (stored != requestFingerprint)
                return setError(QStringLiteral("IDEMPOTENCY_CONFLICT"), error);
            return true;
        }
        sqlite3_finalize(existing);
    }

    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT OR IGNORE INTO event_inbox(event_id, idempotency_key, "
                      "request_fingerprint, target_component_id, message_type, payload, status, created_at) "
                      "VALUES(?, ?, ?, ?, ?, ?, 'accepted', strftime('%s','now'))";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, event.eventId)
        || !bindText(statement, 2, event.idempotencyKey)
        || !bindBytes(statement, 3, requestFingerprint)
        || !bindText(statement, 4, event.targetComponentId)
        || !bindText(statement, 5, event.messageType)
        || !bindBytes(statement, 6, event.payload)
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

QList<PersistedInboxEvent> RuntimePersistence::loadInboxEvents(QString *error) const
{
    QList<PersistedInboxEvent> result;
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return result; }
    sqlite3_stmt *statement = nullptr;
    const char *sql = "SELECT event_id, idempotency_key, target_component_id, message_type, payload, "
                      "status, request_fingerprint, failure_code, failure_message, created_at "
                      "FROM event_inbox ORDER BY created_at, event_id";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK)
    { setError(sqliteError(), error); return result; }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        PersistedInboxEvent item;
        item.event.eventId = columnText(statement, 0);
        item.event.idempotencyKey = columnText(statement, 1);
        item.event.targetComponentId = columnText(statement, 2);
        item.event.messageType = columnText(statement, 3);
        item.event.payload = columnBytes(statement, 4);
        item.status = columnText(statement, 5);
        item.requestFingerprint = columnBytes(statement, 6);
        item.failure.code = columnText(statement, 7);
        item.failure.message = columnText(statement, 8);
        item.failure.eventId = item.event.eventId;
        item.failure.componentId = item.event.targetComponentId;
        item.createdAt = sqlite3_column_int64(statement, 9);
        result.append(item);
    }
    sqlite3_finalize(statement);
    return result;
}

QList<PersistedInboxEvent> RuntimePersistence::loadIncompleteEvents(QString *error) const
{
    QList<PersistedInboxEvent> result;
    const auto all = loadInboxEvents(error);
    for (const auto &item : all)
    {
        if (item.status == QStringLiteral("accepted") || item.status == QStringLiteral("processing"))
            result.append(item);
    }
    return result;
}

bool RuntimePersistence::markEventProcessing(const QString &eventId, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database,
                           "UPDATE event_inbox SET status='processing' WHERE event_id=? AND status='accepted'",
                           -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, eventId) || sqlite3_step(statement) != SQLITE_DONE)
    { if (statement) sqlite3_finalize(statement); return setError(sqliteError(), error); }
    sqlite3_finalize(statement);
    return true;
}

bool RuntimePersistence::beginTransaction(QString *error)
{
    return execute(QStringLiteral("BEGIN IMMEDIATE TRANSACTION;"), error);
}

bool RuntimePersistence::commitTransaction(QString *error)
{
    return execute(QStringLiteral("COMMIT;"), error);
}

bool RuntimePersistence::rollbackTransaction(QString *error)
{
    return execute(QStringLiteral("ROLLBACK;"), error);
}

bool RuntimePersistence::saveEventResult(const QString &eventId, const QString &deploymentId,
                                         const QString &qualifiedPath,
                                         const QString &componentType, const QByteArray &state,
                                         int schemaVersion, bool succeeded,
                                         const RuntimeFailure &failure, QString *error)
{
    if (!beginTransaction(error)) return false;
    if (!saveComponentState(deploymentId, qualifiedPath, componentType, state, schemaVersion, error)
        || !saveComponentState(qualifiedPath, componentType, state, schemaVersion, error)
        || !recordEventCompletion(eventId, succeeded, failure, error))
    {
        rollbackTransaction(nullptr);
        return false;
    }
    if (!commitTransaction(error))
    {
        rollbackTransaction(nullptr);
        return false;
    }
    return true;
}

bool RuntimePersistence::markRuntimeStatus(const QString &status, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "UPDATE runtime_instances SET status=?, updated_at=strftime('%s','now') WHERE runtime_id=?";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, status) || !bindText(statement, 2, m_runtimeId)
        || sqlite3_step(statement) != SQLITE_DONE)
    { if (statement) sqlite3_finalize(statement); return setError(sqliteError(), error); }
    sqlite3_finalize(statement);
    return true;
}

QString RuntimePersistence::runtimeStatus(QString *error) const
{
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return {}; }
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database, "SELECT status FROM runtime_instances WHERE runtime_id=?",
                           -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, m_runtimeId))
    { if (statement) sqlite3_finalize(statement); setError(sqliteError(), error); return {}; }
    const bool found = sqlite3_step(statement) == SQLITE_ROW;
    const QString status = found ? columnText(statement, 0) : QString();
    sqlite3_finalize(statement);
    if (!found) setError(QStringLiteral("Runtime instance not found."), error);
    return status;
}

bool RuntimePersistence::recordEventCompletion(const QString &eventId, bool succeeded,
                                               const RuntimeFailure &failure, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "UPDATE event_inbox SET status=?, failure_code=?, failure_message=? WHERE event_id=?";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, succeeded ? QStringLiteral("completed") : QStringLiteral("failed"))
        || !bindText(statement, 2, failure.code)
        || !bindText(statement, 3, failure.message)
        || !bindText(statement, 4, eventId)
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);

    const char *historySql = "INSERT INTO event_history(event_id, status, failure_code, failure_message, created_at) "
                             "VALUES(?, ?, ?, ?, strftime('%s','now'))";
    if (sqlite3_prepare_v2(m_database, historySql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, eventId)
        || !bindText(statement, 2, succeeded ? QStringLiteral("completed") : QStringLiteral("failed"))
        || !bindText(statement, 3, failure.code)
        || !bindText(statement, 4, failure.message)
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

bool RuntimePersistence::recordRejection(const RejectionRecord &record, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT INTO event_rejections(event_id, idempotency_key, target_component_id, "
                      "route_id, reason_code, mailbox_depth, created_at) VALUES(?, ?, ?, ?, ?, ?, strftime('%s','now'))";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, record.eventId)
        || !bindText(statement, 2, record.idempotencyKey)
        || !bindText(statement, 3, record.targetComponentId)
        || !bindText(statement, 4, record.routeId)
        || !bindText(statement, 5, record.reasonCode)
        || sqlite3_bind_int64(statement, 6, record.mailboxDepth) != SQLITE_OK
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

bool RuntimePersistence::recordFailure(const FailureRecord &record, QString *error)
{
    return recordLog(QStringLiteral("error"), record.reasonCode, record.failure.message, error);
}

bool RuntimePersistence::recordLog(const QString &level, const QString &code,
                                   const QString &message, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT INTO runtime_logs(level, code, message, created_at) VALUES(?, ?, ?, strftime('%s','now'))";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, level)
        || !bindText(statement, 2, code)
        || !bindText(statement, 3, message)
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

bool RuntimePersistence::recordDiscoveryLifecycleEvent(const DiscoveryEvent &event,
                                                       const QString &contractVersion,
                                                       QString *error)
{
    if (!m_database)
        return setError(QStringLiteral("Database is not open."), error);

    if (event.eventId.trimmed().isEmpty() || event.component.sourceId.trimmed().isEmpty())
        return setError(QStringLiteral("Discovery lifecycle event is invalid."), error);

    sqlite3_stmt *statement = nullptr;
    const char *sql =
        "INSERT OR IGNORE INTO discovery_lifecycle_events("
        "event_id, source_id, component_identity, version, kind, status, occurred_at, contract_version) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, event.eventId)
        || !bindText(statement, 2, event.component.sourceId)
        || !bindText(statement, 3, event.component.identity)
        || !bindText(statement, 4, event.component.version)
        || !bindText(statement, 5, discoveryKindName(event.kind))
        || !bindText(statement, 6, discoveryStatusName(event.status))
        || sqlite3_bind_int64(statement, 7,
                              event.timestamp.isValid()
                                  ? event.timestamp.toMSecsSinceEpoch()
                                  : QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()) != SQLITE_OK
        || bindNullableText(statement, 8, contractVersion) == false
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);

    const qint64 occurredAt = event.timestamp.isValid()
                                  ? event.timestamp.toMSecsSinceEpoch()
                                  : QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    for (const auto &diagnostic : event.diagnostics)
    {
        sqlite3_stmt *diagnosticStatement = nullptr;
        const char *diagnosticSql =
            "INSERT OR IGNORE INTO discovery_lifecycle_diagnostics("
            "event_id, code, message, field_path, retryable) VALUES(?, ?, ?, ?, ?)";
        if (sqlite3_prepare_v2(m_database, diagnosticSql, -1, &diagnosticStatement, nullptr) != SQLITE_OK
            || !bindText(diagnosticStatement, 1, event.eventId)
            || !bindText(diagnosticStatement, 2, diagnostic.code)
            || !bindText(diagnosticStatement, 3, diagnostic.message)
            || bindNullableText(diagnosticStatement, 4, diagnostic.fieldPath) == false
            || sqlite3_bind_int(diagnosticStatement, 5, diagnostic.retryable ? 1 : 0) != SQLITE_OK
            || sqlite3_step(diagnosticStatement) != SQLITE_DONE)
        {
            if (diagnosticStatement) sqlite3_finalize(diagnosticStatement);
            return setError(sqliteError(), error);
        }
        sqlite3_finalize(diagnosticStatement);
    }
    Q_UNUSED(occurredAt);
    return true;
}

PersistedDiscoveryLifecycleHistory RuntimePersistence::discoveryLifecycleHistory(
    const PersistedDiscoveryLifecycleQuery &query, QString *error) const
{
    PersistedDiscoveryLifecycleHistory result;
    if (!m_database)
    {
        setError(QStringLiteral("Database is not open."), error);
        return result;
    }

    const int limit = qBound(1, query.limit, 1000);
    const int offset = qMax(0, query.offset);
    sqlite3_stmt *statement = nullptr;
    const char *sql =
        "SELECT event_id, source_id, component_identity, version, kind, status, occurred_at, contract_version "
        "FROM discovery_lifecycle_events "
        "WHERE (? = '' OR source_id = ?) AND (? = '' OR component_identity = ?) "
        "AND (? = '' OR version = ?) AND (? = '' OR kind = ?) "
        "ORDER BY occurred_at ASC, id ASC LIMIT ? OFFSET ?";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, query.sourceId) || !bindText(statement, 2, query.sourceId)
        || !bindText(statement, 3, query.componentIdentity) || !bindText(statement, 4, query.componentIdentity)
        || !bindText(statement, 5, query.version) || !bindText(statement, 6, query.version)
        || !bindText(statement, 7, query.kind) || !bindText(statement, 8, query.kind)
        || sqlite3_bind_int(statement, 9, limit) != SQLITE_OK
        || sqlite3_bind_int(statement, 10, offset) != SQLITE_OK)
    {
        if (statement) sqlite3_finalize(statement);
        setError(sqliteError(), error);
        return result;
    }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        PersistedDiscoveryLifecycleEvent event;
        event.eventId = columnText(statement, 0);
        event.sourceId = columnText(statement, 1);
        event.componentIdentity = columnText(statement, 2);
        event.version = columnText(statement, 3);
        event.kind = columnText(statement, 4);
        event.status = columnText(statement, 5);
        event.occurredAt = sqlite3_column_int64(statement, 6);
        event.contractVersion = columnText(statement, 7);
        result.events.append(event);
    }
    sqlite3_finalize(statement);

    for (auto &event : result.events)
    {
        sqlite3_stmt *diagnosticStatement = nullptr;
        const char *diagnosticSql =
            "SELECT code, message, field_path, retryable FROM discovery_lifecycle_diagnostics "
            "WHERE event_id = ? ORDER BY id ASC";
        if (sqlite3_prepare_v2(m_database, diagnosticSql, -1, &diagnosticStatement, nullptr) != SQLITE_OK
            || !bindText(diagnosticStatement, 1, event.eventId))
        {
            if (diagnosticStatement) sqlite3_finalize(diagnosticStatement);
            setError(sqliteError(), error);
            return {};
        }
        while (sqlite3_step(diagnosticStatement) == SQLITE_ROW)
            event.diagnostics.append({columnText(diagnosticStatement, 0), columnText(diagnosticStatement, 1),
                                      columnText(diagnosticStatement, 2), sqlite3_column_int(diagnosticStatement, 3) != 0});
        sqlite3_finalize(diagnosticStatement);
    }
    if (result.events.size() == limit)
        result.nextOffset = offset + limit;
    return result;
}

QList<PersistedEventHistory> RuntimePersistence::eventHistory(int limit, QString *error) const
{
    QList<PersistedEventHistory> items;
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return items; }
    sqlite3_stmt *statement = nullptr;
    const char *sql = "SELECT event_id, status, failure_code, failure_message, created_at "
                      "FROM event_history ORDER BY id DESC LIMIT ?";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || sqlite3_bind_int(statement, 1, qMax(0, limit)) != SQLITE_OK)
    { setError(sqliteError(), error); if (statement) sqlite3_finalize(statement); return items; }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        items.append({columnText(statement, 0), columnText(statement, 1), columnText(statement, 2),
                      columnText(statement, 3), sqlite3_column_int64(statement, 4)});
    }
    sqlite3_finalize(statement);
    return items;
}

QList<PersistedRuntimeLog> RuntimePersistence::runtimeLogs(int limit, QString *error) const
{
    QList<PersistedRuntimeLog> items;
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return items; }
    sqlite3_stmt *statement = nullptr;
    const char *sql = "SELECT level, code, message, created_at FROM runtime_logs "
                      "ORDER BY id DESC LIMIT ?";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || sqlite3_bind_int(statement, 1, qMax(0, limit)) != SQLITE_OK)
    { setError(sqliteError(), error); if (statement) sqlite3_finalize(statement); return items; }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        items.append({columnText(statement, 0), columnText(statement, 1), columnText(statement, 2),
                      sqlite3_column_int64(statement, 3)});
    }
    sqlite3_finalize(statement);
    return items;
}

int RuntimePersistence::eventHistoryCount(QString *error) const
{
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return 0; }
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database, "SELECT COUNT(*) FROM event_history", -1, &statement, nullptr) != SQLITE_OK)
    { setError(sqliteError(), error); return 0; }
    const int count = sqlite3_step(statement) == SQLITE_ROW ? sqlite3_column_int(statement, 0) : 0;
    sqlite3_finalize(statement);
    return count;
}

int RuntimePersistence::rejectionCount(QString *error) const
{
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return 0; }
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database, "SELECT COUNT(*) FROM event_rejections", -1, &statement, nullptr) != SQLITE_OK)
    { setError(sqliteError(), error); return 0; }
    const int count = sqlite3_step(statement) == SQLITE_ROW ? sqlite3_column_int(statement, 0) : 0;
    sqlite3_finalize(statement);
    return count;
}

int RuntimePersistence::logCount(QString *error) const
{
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return 0; }
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database, "SELECT COUNT(*) FROM runtime_logs", -1, &statement, nullptr) != SQLITE_OK)
    { setError(sqliteError(), error); return 0; }
    const int count = sqlite3_step(statement) == SQLITE_ROW ? sqlite3_column_int(statement, 0) : 0;
    sqlite3_finalize(statement);
    return count;
}

int RuntimePersistence::pendingEventCount(QString *error) const
{
    if (!m_database) { setError(QStringLiteral("Database is not open."), error); return 0; }
    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(m_database, "SELECT COUNT(*) FROM event_inbox WHERE status='accepted'", -1, &statement, nullptr) != SQLITE_OK)
    { setError(sqliteError(), error); return 0; }
    const int count = sqlite3_step(statement) == SQLITE_ROW ? sqlite3_column_int(statement, 0) : 0;
    sqlite3_finalize(statement);
    return count;
}

bool RuntimePersistence::resetExecutionState(QString *error)
{
    return execute(QStringLiteral("DELETE FROM component_state; DELETE FROM event_inbox;"), error);
}

bool RuntimePersistence::execute(const QString &sql, QString *error) const
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    char *message = nullptr;
    const QByteArray statement = sql.toUtf8();
    const int result = sqlite3_exec(m_database, statement.constData(), nullptr, nullptr, &message);
    if (result != SQLITE_OK)
    {
        const QString errorMessage = message ? QString::fromUtf8(message) : sqliteError();
        sqlite3_free(message);
        return setError(errorMessage, error);
    }
    return true;
}

bool RuntimePersistence::migrate(QString *error)
{
    return execute(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS runtime_instances(runtime_id TEXT PRIMARY KEY, status TEXT NOT NULL, updated_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS deployment_snapshots(deployment_id TEXT PRIMARY KEY, root_graph_id TEXT NOT NULL, root_graph_version TEXT NOT NULL, dependency_manifest TEXT NOT NULL, definition_hashes TEXT NOT NULL, execution_plan_hash TEXT NOT NULL, serialized_definitions BLOB NOT NULL, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS component_state_v2(deployment_id TEXT NOT NULL, qualified_path TEXT NOT NULL, component_type TEXT NOT NULL, state_blob BLOB NOT NULL, schema_version INTEGER NOT NULL, updated_at INTEGER NOT NULL, PRIMARY KEY(deployment_id, qualified_path));"
        "CREATE TABLE IF NOT EXISTS graph_versions(graph_version TEXT PRIMARY KEY, graph_blob BLOB NOT NULL, active INTEGER NOT NULL DEFAULT 0, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS component_state(instance_id TEXT PRIMARY KEY, component_type TEXT NOT NULL, state_blob BLOB NOT NULL, schema_version INTEGER NOT NULL, updated_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS event_inbox(event_id TEXT PRIMARY KEY, idempotency_key TEXT, request_fingerprint BLOB, target_component_id TEXT NOT NULL, message_type TEXT NOT NULL, payload BLOB, status TEXT NOT NULL CHECK(status IN ('accepted','processing','completed','failed','rejected')), failure_code TEXT, failure_message TEXT, created_at INTEGER NOT NULL);"
        "CREATE INDEX IF NOT EXISTS idx_event_inbox_status ON event_inbox(status);"
        "CREATE INDEX IF NOT EXISTS idx_event_inbox_idempotency ON event_inbox(idempotency_key);"
        "CREATE TABLE IF NOT EXISTS event_outbox(id INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT NOT NULL, target_component_id TEXT NOT NULL, message_type TEXT NOT NULL, payload BLOB, status TEXT NOT NULL DEFAULT 'pending', created_at INTEGER NOT NULL);"
        "CREATE INDEX IF NOT EXISTS idx_event_outbox_status ON event_outbox(status);"
        "CREATE TABLE IF NOT EXISTS event_history(id INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT NOT NULL, status TEXT NOT NULL, failure_code TEXT, failure_message TEXT, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS component_history(id INTEGER PRIMARY KEY AUTOINCREMENT, deployment_id TEXT, qualified_path TEXT NOT NULL, state_blob BLOB NOT NULL, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS event_rejections(id INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT, idempotency_key TEXT, target_component_id TEXT, route_id TEXT, reason_code TEXT NOT NULL, mailbox_depth INTEGER NOT NULL, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS runtime_logs(id INTEGER PRIMARY KEY AUTOINCREMENT, level TEXT NOT NULL, code TEXT NOT NULL, message TEXT NOT NULL, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS discovery_lifecycle_events(id INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT NOT NULL UNIQUE, source_id TEXT NOT NULL, component_identity TEXT NOT NULL, version TEXT NOT NULL, kind TEXT NOT NULL, status TEXT NOT NULL, occurred_at INTEGER NOT NULL, contract_version TEXT NOT NULL DEFAULT '');"
        "CREATE INDEX IF NOT EXISTS idx_discovery_lifecycle_time ON discovery_lifecycle_events(occurred_at, id);"
        "CREATE INDEX IF NOT EXISTS idx_discovery_lifecycle_component ON discovery_lifecycle_events(source_id, component_identity, version, kind);"
        "CREATE TABLE IF NOT EXISTS discovery_lifecycle_diagnostics(id INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT NOT NULL, code TEXT NOT NULL, message TEXT NOT NULL, field_path TEXT, retryable INTEGER NOT NULL DEFAULT 0, UNIQUE(event_id, code, message, field_path), FOREIGN KEY(event_id) REFERENCES discovery_lifecycle_events(event_id) ON DELETE CASCADE);"), error); /* migration continues below */

    if (!m_database) return false;
    sqlite3_stmt *columns = nullptr;
    bool hasFingerprint = false;
    if (sqlite3_prepare_v2(m_database, "PRAGMA table_info(event_inbox)", -1, &columns, nullptr) != SQLITE_OK)
        return setError(sqliteError(), error);
    while (sqlite3_step(columns) == SQLITE_ROW)
    {
        if (columnText(columns, 1) == QStringLiteral("request_fingerprint"))
            hasFingerprint = true;
    }
    sqlite3_finalize(columns);
    if (!hasFingerprint
        && !execute(QStringLiteral("ALTER TABLE event_inbox ADD COLUMN request_fingerprint BLOB;"), error))
        return false;
    return true;
}

bool RuntimePersistence::setError(const QString &message, QString *error) const
{
    if (error) *error = message;
    return false;
}

QString RuntimePersistence::sqliteError() const
{
    return m_database ? QString::fromUtf8(sqlite3_errmsg(m_database))
                      : QStringLiteral("SQLite database is not open.");
}

} // namespace cme::runtime
