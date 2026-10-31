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

bool RuntimePersistence::recordAcceptedEvent(const RuntimeEvent &event, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "INSERT OR IGNORE INTO event_inbox(event_id, idempotency_key, target_component_id, "
                      "message_type, payload, status, created_at) VALUES(?, ?, ?, ?, ?, 'accepted', strftime('%s','now'))";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, event.eventId)
        || !bindText(statement, 2, event.idempotencyKey)
        || !bindText(statement, 3, event.targetComponentId)
        || !bindText(statement, 4, event.messageType)
        || !bindBytes(statement, 5, event.payload)
        || sqlite3_step(statement) != SQLITE_DONE)
    {
        if (statement) sqlite3_finalize(statement);
        return setError(sqliteError(), error);
    }
    sqlite3_finalize(statement);
    return true;
}

bool RuntimePersistence::recordEventCompletion(const QString &eventId, bool succeeded,
                                               const RuntimeFailure &failure, QString *error)
{
    if (!m_database) return setError(QStringLiteral("Database is not open."), error);
    sqlite3_stmt *statement = nullptr;
    const char *sql = "UPDATE event_inbox SET status=? WHERE event_id=?";
    if (sqlite3_prepare_v2(m_database, sql, -1, &statement, nullptr) != SQLITE_OK
        || !bindText(statement, 1, succeeded ? QStringLiteral("completed") : QStringLiteral("failed"))
        || !bindText(statement, 2, eventId)
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
        "CREATE TABLE IF NOT EXISTS graph_versions(graph_version TEXT PRIMARY KEY, graph_blob BLOB NOT NULL, active INTEGER NOT NULL DEFAULT 0, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS component_state(instance_id TEXT PRIMARY KEY, component_type TEXT NOT NULL, state_blob BLOB NOT NULL, schema_version INTEGER NOT NULL, updated_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS event_inbox(event_id TEXT PRIMARY KEY, idempotency_key TEXT, target_component_id TEXT NOT NULL, message_type TEXT NOT NULL, payload BLOB, status TEXT NOT NULL, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS event_history(id INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT NOT NULL, status TEXT NOT NULL, failure_code TEXT, failure_message TEXT, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS event_rejections(id INTEGER PRIMARY KEY AUTOINCREMENT, event_id TEXT, idempotency_key TEXT, target_component_id TEXT, route_id TEXT, reason_code TEXT NOT NULL, mailbox_depth INTEGER NOT NULL, created_at INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS runtime_logs(id INTEGER PRIMARY KEY AUTOINCREMENT, level TEXT NOT NULL, code TEXT NOT NULL, message TEXT NOT NULL, created_at INTEGER NOT NULL);"), error);
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
