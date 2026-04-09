#include "CommandGateway.h"

#include "CapabilityRegistry.h"
#include "InvariantChecker.h"
#include "adapters/CommandAdapter.h"
#include "adapters/CommandAdapterRegistry.h"
#include "commands/UndoStack.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <algorithm>

namespace {

QVariantMap makeLogEntry(const QString &actor,
                         const QString &commandType,
                         bool blocked,
                         const QString &reason)
{
    QVariantMap entry;
    entry.insert(QStringLiteral("actor"),       actor);
    entry.insert(QStringLiteral("command"),     commandType);
    entry.insert(QStringLiteral("blocked"),     blocked);
    entry.insert(QStringLiteral("reason"),      reason);
    entry.insert(QStringLiteral("timestampMs"), QDateTime::currentMSecsSinceEpoch());
    return entry;
}

} // namespace

// ── Construction ──────────────────────────────────────────────────────────────

CommandGateway::CommandGateway(QObject *parent)
    : QObject(parent),
      m_adapterRegistry(std::make_unique<CommandAdapterRegistry>())
{}

// ── Properties ────────────────────────────────────────────────────────────────

GraphModel *CommandGateway::graph() const { return m_graph; }

void CommandGateway::setGraph(GraphModel *graph)
{
    if (m_graph == graph)
        return;
    m_graph = graph;
    emit graphChanged();
}

UndoStack *CommandGateway::undoStack() const { return m_undoStack; }

void CommandGateway::setUndoStack(UndoStack *stack)
{
    if (m_undoStack == stack)
        return;
    m_undoStack = stack;
    emit undoStackChanged();
}

CapabilityRegistry *CommandGateway::capabilityRegistry() const
{
    return m_capabilityRegistry;
}

void CommandGateway::setCapabilityRegistry(CapabilityRegistry *registry)
{
    m_capabilityRegistry = registry;
}

InvariantChecker *CommandGateway::invariantChecker() const
{
    return m_invariantChecker;
}

void CommandGateway::setInvariantChecker(InvariantChecker *checker)
{
    m_invariantChecker = checker;
}

// ── Public dispatch interface ─────────────────────────────────────────────────

bool CommandGateway::executeRequest(const QString &extensionId,
                                    const QVariantMap &commandRequest,
                                    QString *error)
{
    return dispatchCommand(extensionId, commandRequest, /*requireCapability=*/true, error);
}

bool CommandGateway::executeTypedRequest(const QString &extensionId,
                                         const cme::GraphCommandRequest &commandRequest,
                                         QString *error)
{
    const QVariantMap legacyRequest = cme::adapter::graphCommandRequestToVariantMap(commandRequest);
    return executeRequest(extensionId, legacyRequest, error);
}

bool CommandGateway::executeSystemCommand(const QVariantMap &commandRequest, QString *error)
{
    return dispatchCommand(QStringLiteral("__system__"), commandRequest,
                           /*requireCapability=*/false, error);
}

bool CommandGateway::executeTypedSystemCommand(const cme::GraphCommandRequest &commandRequest,
                                               QString *error)
{
    const QVariantMap legacyRequest = cme::adapter::graphCommandRequestToVariantMap(commandRequest);
    return executeSystemCommand(legacyRequest, error);
}

// ── Audit log ─────────────────────────────────────────────────────────────────

QVariantList CommandGateway::requestLog() const { return m_requestLog; }

void CommandGateway::clearRequestLog() { m_requestLog.clear(); }

int CommandGateway::maxLatencySamples() const
{
    return m_maxLatencySamples;
}

void CommandGateway::setMaxLatencySamples(int value)
{
    if (value <= 0 || value == m_maxLatencySamples)
        return;

    m_maxLatencySamples = value;
    while (m_latencySamplesMs.size() > m_maxLatencySamples)
        m_latencySamplesMs.removeFirst();
}

double CommandGateway::commandLatencyP50Ms() const
{
    return percentile(m_latencySamplesMs, 0.50);
}

double CommandGateway::commandLatencyP95Ms() const
{
    return percentile(m_latencySamplesMs, 0.95);
}

double CommandGateway::lastCommandLatencyMs() const
{
    return m_lastCommandLatencyMs;
}

int CommandGateway::commandLatencySampleCount() const
{
    return m_latencySamplesMs.size();
}

void CommandGateway::resetCommandLatencyStats()
{
    m_latencySamplesMs.clear();
    m_lastCommandLatencyMs = 0.0;
}

QStringList CommandGateway::supportedCommands()
{
    // String names are derived from the CommandType enum via the adapter.
    // Core logic must not hard-code these strings.
    static QStringList s_list;
    if (s_list.isEmpty()) {
        using CT = cme::CommandType;
        for (CT t : { CT::COMMAND_TYPE_ADD_COMPONENT,
                      CT::COMMAND_TYPE_REMOVE_COMPONENT,
                      CT::COMMAND_TYPE_MOVE_COMPONENT,
                      CT::COMMAND_TYPE_ADD_CONNECTION,
                      CT::COMMAND_TYPE_REMOVE_CONNECTION,
                      CT::COMMAND_TYPE_SET_COMPONENT_PROPERTY,
                      CT::COMMAND_TYPE_SET_CONNECTION_PROPERTY }) {
            s_list.append(cme::adapter::commandTypeToString(t));
        }
    }
    return s_list;
}

// ── Private implementation ────────────────────────────────────────────────────

void CommandGateway::appendLog(const QString &actor,
                               const QString &commandType,
                               bool blocked,
                               const QString &reason)
{
    m_requestLog.append(makeLogEntry(actor, commandType, blocked, reason));
}

bool CommandGateway::dispatchCommand(const QString &actor,
                                     const QVariantMap &commandRequest,
                                     bool requireCapability,
                                     QString *error)
{
    QElapsedTimer timer;
    timer.start();

    // Helper that records the rejection and returns false.
    auto reject = [&](const QString &reason) -> bool {
        const QString cmd = commandRequest.value(QStringLiteral("command")).toString();
        appendLog(actor, cmd, /*blocked=*/true, reason);
        if (error)
            *error = reason;
        emit mutationBlocked(actor, reason);
        recordLatencySampleNs(timer.nsecsElapsed());
        return false;
    };

    if (!m_graph)
        return reject(QStringLiteral("CommandGateway: no graph attached"));
    if (!m_undoStack)
        return reject(QStringLiteral("CommandGateway: no undo stack attached"));

    const QString commandType =
        commandRequest.value(QStringLiteral("command")).toString().trimmed();
    if (commandType.isEmpty())
        return reject(QStringLiteral("CommandGateway: commandRequest missing 'command' key"));

    // Convert the legacy string discriminator to a typed enum exactly once at the boundary.
    // All dispatch below uses the enum; string literals remain confined to CommandAdapter.
    const cme::CommandType cmdEnum = cme::adapter::commandTypeFromString(commandType);

    // ── Capability gate ───────────────────────────────────────────────────
    if (requireCapability && m_capabilityRegistry) {
        if (!m_capabilityRegistry->checkAndAudit(
                actor, QStringLiteral("graph.mutate"),
                QStringLiteral("CommandGateway::executeRequest[%1]").arg(commandType))) {
            return reject(
                QStringLiteral("Extension '%1' lacks 'graph.mutate' capability").arg(actor));
        }
    }

    // ── Pre-command invariant check ───────────────────────────────────────
    {
        QString violation;
        if (!runPreChecks(commandType, &violation)) {
            if (error)
                *error = violation;
            appendLog(actor, commandType, /*blocked=*/true, violation);
            emit mutationBlocked(actor, violation);
            return false;
        }
    }

    // ── Command dispatch → UndoStack ──────────────────────────────────────
    // Each branch validates its own required fields and returns reject() on
    // missing data.  On success it pushes exactly one command to the undo stack.

    switch (cmdEnum) {
    case cme::COMMAND_TYPE_ADD_COMPONENT: {
        const QString id     = commandRequest.value(QStringLiteral("id")).toString();
        const QString typeId = commandRequest.value(QStringLiteral("typeId")).toString();
        const qreal   x      = commandRequest.value(QStringLiteral("x"), 0.0).toDouble();
        const qreal   y      = commandRequest.value(QStringLiteral("y"), 0.0).toDouble();

        if (id.isEmpty())
            return reject(QStringLiteral("addComponent: 'id' is required"));
        if (m_graph->componentById(id))
            return reject(
                QStringLiteral("addComponent: component '%1' already exists").arg(id));

        auto *component = new ComponentModel(m_graph);
        component->setId(id);
        component->setType(typeId);
        component->setX(x);
        component->setY(y);
        m_undoStack->pushAddComponent(m_graph, component);
        break;
    }
    case cme::COMMAND_TYPE_REMOVE_COMPONENT: {
        const QString id = commandRequest.value(QStringLiteral("id")).toString();
        if (id.isEmpty())
            return reject(QStringLiteral("removeComponent: 'id' is required"));
        if (!m_graph->componentById(id))
            return reject(
                QStringLiteral("removeComponent: component '%1' not found").arg(id));
        m_undoStack->pushRemoveComponent(m_graph, id);
        break;
    }
    case cme::COMMAND_TYPE_MOVE_COMPONENT: {
        const QString id = commandRequest.value(QStringLiteral("id")).toString();
        const qreal   nx = commandRequest.value(QStringLiteral("x"), 0.0).toDouble();
        const qreal   ny = commandRequest.value(QStringLiteral("y"), 0.0).toDouble();
        if (id.isEmpty())
            return reject(QStringLiteral("moveComponent: 'id' is required"));
        ComponentModel *comp = m_graph->componentById(id);
        if (!comp)
            return reject(QStringLiteral("moveComponent: component '%1' not found").arg(id));
        m_undoStack->pushMoveComponent(m_graph, id, comp->x(), comp->y(), nx, ny);
        break;
    }
    case cme::COMMAND_TYPE_ADD_CONNECTION: {
        const QString id    = commandRequest.value(QStringLiteral("id")).toString();
        const QString src   = commandRequest.value(QStringLiteral("sourceId")).toString();
        const QString tgt   = commandRequest.value(QStringLiteral("targetId")).toString();
        const QString label = commandRequest.value(QStringLiteral("label")).toString();

        if (id.isEmpty())
            return reject(QStringLiteral("addConnection: 'id' is required"));
        if (src.isEmpty() || tgt.isEmpty())
            return reject(QStringLiteral("addConnection: 'sourceId' and 'targetId' are required"));
        if (!m_graph->componentById(src))
            return reject(
                QStringLiteral("addConnection: source component '%1' not found").arg(src));
        if (!m_graph->componentById(tgt))
            return reject(
                QStringLiteral("addConnection: target component '%1' not found").arg(tgt));

        auto *connection = new ConnectionModel(m_graph);
        connection->setId(id);
        connection->setSourceId(src);
        connection->setTargetId(tgt);
        connection->setLabel(label);
        m_undoStack->pushAddConnection(m_graph, connection);
        break;
    }
    case cme::COMMAND_TYPE_REMOVE_CONNECTION: {
        const QString id = commandRequest.value(QStringLiteral("id")).toString();
        if (id.isEmpty())
            return reject(QStringLiteral("removeConnection: 'id' is required"));
        m_undoStack->pushRemoveConnection(m_graph, id);
        break;
    }
    case cme::COMMAND_TYPE_SET_COMPONENT_PROPERTY: {
        const QString  id       = commandRequest.value(QStringLiteral("id")).toString();
        const QString  property = commandRequest.value(QStringLiteral("property")).toString();
        const QVariant value    = commandRequest.value(QStringLiteral("value"));
        if (id.isEmpty() || property.isEmpty())
            return reject(
                QStringLiteral("setComponentProperty: 'id' and 'property' are required"));
        ComponentModel *comp = m_graph->componentById(id);
        if (!comp)
            return reject(
                QStringLiteral("setComponentProperty: component '%1' not found").arg(id));
        m_undoStack->pushSetComponentProperty(comp, property, value);
        break;
    }
    case cme::COMMAND_TYPE_SET_CONNECTION_PROPERTY: {
        const QString  id       = commandRequest.value(QStringLiteral("id")).toString();
        const QString  property = commandRequest.value(QStringLiteral("property")).toString();
        const QVariant value    = commandRequest.value(QStringLiteral("value"));
        if (id.isEmpty() || property.isEmpty())
            return reject(
                QStringLiteral("setConnectionProperty: 'id' and 'property' are required"));
        ConnectionModel *conn = m_graph->connectionById(id);
        if (!conn)
            return reject(
                QStringLiteral("setConnectionProperty: connection '%1' not found").arg(id));
        m_undoStack->pushSetConnectionProperty(conn, property, value);
        break;
    }
    default:
        return reject(QStringLiteral("CommandGateway: unknown command '%1'").arg(commandType));
    }

    // ── Post-command invariant check with rollback ────────────────────────
    {
        QString violation;
        if (!runPostChecks(commandType, &violation)) {
            // Roll back the command we just pushed.
            if (m_undoStack->canUndo())
                m_undoStack->undo();
            m_undoStack->discardRedoHistory();

            appendLog(actor, commandType, /*blocked=*/true,
                      QStringLiteral("Post-command invariant violation (rolled back): %1")
                          .arg(violation));
            if (error)
                *error = violation;
            emit invariantViolationRolledBack(commandType, violation);
            emit mutationBlocked(actor, violation);
            return false;
        }
    }

    appendLog(actor, commandType, /*blocked=*/false, {});
    emit commandExecuted(actor, commandType);
    recordLatencySampleNs(timer.nsecsElapsed());
    return true;
}

void CommandGateway::recordLatencySampleNs(qint64 elapsedNs)
{
    const double elapsedMs = static_cast<double>(elapsedNs) / 1000000.0;
    m_lastCommandLatencyMs = elapsedMs;

    m_latencySamplesMs.append(elapsedMs);
    while (m_latencySamplesMs.size() > m_maxLatencySamples)
        m_latencySamplesMs.removeFirst();
}

double CommandGateway::percentile(const QVector<double> &samples, double p)
{
    if (samples.isEmpty())
        return 0.0;

    QVector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const int idx = qBound(0, static_cast<int>(p * (sorted.size() - 1)), sorted.size() - 1);
    return sorted.at(idx);
}

bool CommandGateway::runPreChecks(const QString &commandType, QString *error) const
{
    if (!m_invariantChecker || !m_graph)
        return true;

    QString violation;
    if (!m_invariantChecker->checkAll(m_graph, &violation)) {
        const QString msg =
            QStringLiteral("Pre-command invariant violation [%1]: %2")
                .arg(commandType, violation);
        if (error)
            *error = msg;
        return false;
    }
    return true;
}

bool CommandGateway::runPostChecks(const QString &commandType, QString *error)
{
    if (!m_invariantChecker || !m_graph)
        return true;

    QString violation;
    if (!m_invariantChecker->checkAll(m_graph, &violation)) {
        const QString msg =
            QStringLiteral("Post-command invariant violation [%1]: %2")
                .arg(commandType, violation);
        if (error)
            *error = msg;
        return false;
    }
    return true;
}
