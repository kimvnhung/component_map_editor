#include "SequentialExecutionEngine.h"

#include "extensions/contracts/builders/ExecutionPlanBuilder.h"
#include "services/ExecutionMigrationFlags.h"
#include "services/execution_internal/ProviderResultValidation.h"
#include "utils/GraphHelper.h"
#include "adapters/GraphAdapter.h"

SequentialExecutionEngine::SequentialExecutionEngine() {}


bool SequentialExecutionEngine::prepare(GraphModel *graph,
                                        const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                                        const ExecutionRequest &request,
                                        QString *error)
{
    reset();

    if (!graph)
    {
        if (error)
        {
            *error = QStringLiteral("Graph is not set.");
        }

        return false;
    }

    m_providerByComponentType = providersByType;
    m_inputSnapshot = request.inputSnapshot;

    if (!ExecutionPlanBuilder::build(graph, request, &m_executionPlan, error))
    {
        reset();
        return false;
    }

    const QSet<QString> &activeComponentIds = m_executionPlan.reachableComponentIds;
    const QList<ComponentModel *> components = graph->componentList();

    for (ComponentModel *component : components)
    {
        if (!component)
        {
            if (error)
            {
                *error = QStringLiteral("Graph contains a null component.");
            }

            reset();
            return false;
        }

        const QString componentId = component->id().trimmed();
        if (!activeComponentIds.contains(componentId))
        {
            continue;
        }

        cme::ComponentData *snap = m_graphSnapshot.add_components();
        snap->set_id(componentId.toStdString());
        snap->set_type_id(component->type().toStdString());
        snap->set_title(component->title().toStdString());

        auto *properties = snap->mutable_properties();
        (*properties)["x"] = QString::number(component->x()).toStdString();
        (*properties)["y"] = QString::number(component->y()).toStdString();
        (*properties)["width"] = QString::number(component->width()).toStdString();
        (*properties)["height"] = QString::number(component->height()).toStdString();
        (*properties)["color"] = component->color().toStdString();
        (*properties)["shape"] = component->shape().toStdString();

        const QList<QByteArray> dynamicProps = component->dynamicPropertyNames();
        for (const QByteArray &propName : dynamicProps)
        {
            const QString key = QString::fromUtf8(propName);
            if (!key.isEmpty())
            {
                (*properties)[key.toStdString()] = component->property(propName.constData()).toString().toStdString();
            }
        }
    }

    for (const auto &component : m_graphSnapshot.components())
    {
        m_pendingInDegree.insert(component.id(), 0);
    }

    for (ConnectionModel *connection : graph->connectionList())
    {
        if (!connection)
        {
            if (error)
            {
                *error = QStringLiteral("Graph contains a null connection.");
            }

            reset();
            return false;
        }

        const QString connectionId = connection->id().trimmed();
        const QString sourceId = connection->sourceId().trimmed();
        const QString targetId = connection->targetId().trimmed();

        if (!m_executionPlan.activeConnectionIds.contains(connectionId))
        {
            continue;
        }

        cme::ConnectionData conn;
        conn.set_id(connectionId.toStdString());
        conn.set_source_id(sourceId.toStdString());
        conn.set_target_id(targetId.toStdString());
        conn.set_label(connection->label().toStdString());

        const std::string targetKey = targetId.toStdString();
        m_pendingInDegree[targetKey] = m_pendingInDegree.value(targetKey, 0) + 1;
        m_graphSnapshot.mutable_connections()->Add(std::move(conn));
    }

    auto outgoingMap = cme::helper::graph::getOutgoingConnectionsBySourceId(m_graphSnapshot);
    for (auto it = outgoingMap.begin(); it != outgoingMap.end(); ++it)
    {
        QList<cme::ConnectionData> &edges = it.value();
        std::sort(edges.begin(), edges.end(), [](const cme::ConnectionData &a, const cme::ConnectionData &b)
        {
            if (a.target_id() != b.target_id())
            {
                return a.target_id() < b.target_id();
            }
            return a.id() < b.id();
        });
    }

    auto incomingMap = cme::helper::graph::getIncomingConnectionsByTargetId(m_graphSnapshot);
    for (auto it = incomingMap.begin(); it != incomingMap.end(); ++it)
    {
        QList<cme::ConnectionData> &edges = it.value();
        std::sort(edges.begin(), edges.end(), [](const cme::ConnectionData &a, const cme::ConnectionData &b)
        {
            if (a.source_id() != b.source_id())
            {
                return a.source_id() < b.source_id();
            }
            return a.id() < b.id();
        });
    }

    m_pendingInDegree = {};
    for (auto it = m_executionPlan.pendingInDegree.constBegin(); it != m_executionPlan.pendingInDegree.constEnd(); ++it)
    {
        m_pendingInDegree.insert(it.key().toStdString(), it.value());
    }

    enqueueReadyComponent(m_executionPlan.resolvedStartComponentId);
    return true;
}

bool SequentialExecutionEngine::hasReadyWork() const
{
    return !m_readyQueue.isEmpty();
}

QString SequentialExecutionEngine::peekNextReadyComponentId() const
{
    return m_readyQueue.isEmpty() ? QString() : m_readyQueue.first();
}

QStringList SequentialExecutionEngine::readyComponentIds() const
{
    return m_readyQueue;
}

int SequentialExecutionEngine::executedCount() const
{
    return m_executed.size();
}

int SequentialExecutionEngine::totalComponentCount() const
{
    return m_executionPlan.reachableComponentIds.size();
}

const cme::GraphSnapshot &SequentialExecutionEngine::graphSnapshot() const
{
    return m_graphSnapshot;
}

IExecutionEngine::Telemetry SequentialExecutionEngine::telemetry() const
{
    return Telemetry
    {
        m_payloadBytesRead,
        m_payloadBytesWritten,
        m_maxPayloadBytes,
        m_tokenReadCount,
        m_tokenWriteCount
    };
}

IExecutionEngine::StepOutcome SequentialExecutionEngine::executeNext(const QVariantMap &legacyGlobalState,
        ExecutionContext *outCtx,
        ExecuteResult *outResult,
        QString *error)
{
    if (m_readyQueue.isEmpty())
    {
        return StepOutcome::NoWork;
    }

    const QString componentId = dequeNextComponent();
    cme::ComponentData component = cme::helper::graph::getComponentById(m_graphSnapshot, componentId);
    ExecutionContext ctx;
    ctx.componentId = componentId;
    ctx.tokenRoutingEnabled = cme::execution::MigrationFlags::tokenTransportEnabled();
    prepareIncomingTokens(componentId, ctx, legacyGlobalState);
    ctx.componentType = QString::fromStdString(component.type_id());

    const ExecuteResult result = invokeProvider(ctx);

    if (outCtx)
    {
        *outCtx = ctx;
    }

    if (result.status == ExecuteResult::Status::Error)
    {
        if (outResult)
        {
            *outResult = result;
        }
        if (error)
        {
            *error = result.errorMessage;
        }
        return StepOutcome::ProviderError;
    }

    if (result.status == ExecuteResult::Status::Rejected)
    {
        if (outResult)
        {
            *outResult = result;
        }
        return StepOutcome::Rejected;
    }

    QString validateMessage;
    const bool valid = validateExecutionResult(result, validateMessage);
    if (outResult)
    {
        *outResult = result;
    }
    if (!valid)
    {
        if (error)
        {
            *error = validateMessage;
        }
        return StepOutcome::ValidationError;
    }

    routeOutgoingTokens(ctx, result);
    commitSchedulingState(ctx);
    return StepOutcome::Committed;
}

void SequentialExecutionEngine::reset()
{
    m_graphSnapshot.Clear();
    m_executionPlan = ExecutionPlan{};
    m_pendingInDegree.clear();
    m_executed.clear();
    m_readyQueue.clear();
    m_readyQueueSet.clear();
    m_inputSnapshot.clear();
    m_providerByComponentType.clear();

    m_payloadBytesRead = 0;
    m_payloadBytesWritten = 0;
    m_maxPayloadBytes = 0;
    m_tokenReadCount = 0;
    m_tokenWriteCount = 0;
}

void SequentialExecutionEngine::enqueueReadyComponent(const QString &componentId)
{
    if (componentId.isEmpty()
        || !m_executionPlan.reachableComponentIds.contains(componentId)
        || m_executed.contains(componentId)
        || m_readyQueueSet.contains(componentId))
    {
        return;
    }

    auto it = std::lower_bound(m_readyQueue.begin(), m_readyQueue.end(), componentId, cme::helper::graph::idComparator);
    m_readyQueue.insert(it, componentId);
    m_readyQueueSet.insert(componentId);
}

QString SequentialExecutionEngine::dequeNextComponent()
{
    if (m_readyQueue.isEmpty())
    {
        return QString();
    }

    const QString componentId = m_readyQueue.first();
    m_readyQueue.removeFirst();
    m_readyQueueSet.remove(componentId);
    return componentId;
}

void SequentialExecutionEngine::prepareIncomingTokens(const QString &componentId,
        ExecutionContext &ctx,
        const QVariantMap &legacyGlobalState)
{
    const QList<cme::ConnectionData> incoming = cme::helper::graph::getConnectionsByTargetId(m_graphSnapshot, componentId);
    const bool tokenRoutingEnabled = cme::execution::MigrationFlags::tokenTransportEnabled();

    if (tokenRoutingEnabled)
    {
        for (const cme::ConnectionData &edge : incoming)
        {
            const auto tokenPayload = cme::helper::graph::getConnectionPayloadById(
                m_graphSnapshot, QString::fromStdString(edge.id()));
            ctx.incomingTokens.insert(QString::fromStdString(edge.id()), tokenPayload);
        }

        if (componentId == m_executionPlan.resolvedStartComponentId && !m_inputSnapshot.isEmpty())
        {
            ctx.incomingTokens.insert(QStringLiteral("__graph_input__"), m_inputSnapshot);
        }

        ctx.stepState = cme::helper::graph::mergeIncomingTokens(ctx.incomingTokens);
    }
    else
    {
        ctx.incomingTokens.insert(QStringLiteral("__legacy_global_state__"), legacyGlobalState);
        ctx.stepState = legacyGlobalState;
    }

    QStringList incomingTokenIds = ctx.incomingTokens.keys();
    std::sort(incomingTokenIds.begin(), incomingTokenIds.end());
    for (const QString &tokenId : std::as_const(incomingTokenIds))
    {
        ++m_tokenReadCount;
        const qint64 bytes = cme::helper::graph::estimatePayloadBytes(ctx.incomingTokens.value(tokenId));
        m_payloadBytesRead += bytes;
        m_maxPayloadBytes = qMax(m_maxPayloadBytes, bytes);
    }
}

ExecuteResult SequentialExecutionEngine::invokeProvider(const ExecutionContext &ctx)
{
    ExecuteResult output;
    output.componentData = cme::helper::graph::getComponentById(m_graphSnapshot, ctx.componentId);
    const IExecutionSemanticsProvider *provider = m_providerByComponentType.value(ctx.componentType, nullptr);

    if (provider)
    {
        QString error;
        output.providerId = provider->providerId();
        output.requiredOutputKeys = provider->providedOutputKeys(ctx.componentType);
        QVariantMap componentSnapshot = cme::adapter::componentSnapshot(output.componentData);

        if (!provider->executeComponent(ctx.componentType,
                                        ctx.componentId,
                                        componentSnapshot,
                                        ctx.incomingTokens,
                                        &output.output,
                                        &output.trace,
                                        &error))
        {
            output.status = ExecuteResult::Status::Error;
            output.errorMessage = error;
            return output;
        }
        output.status = ExecuteResult::Status::Ok;
    }
    else
    {
        output.trace.insert(QStringLiteral("provider"), QStringLiteral("default"));
        output.trace.insert(QStringLiteral("note"),
                            QStringLiteral("No execution semantics provider registered for component type."));

        if (ctx.tokenRoutingEnabled)
        {
            const QList<cme::ConnectionData> incoming = cme::helper::graph::getConnectionsByTargetId(
                m_graphSnapshot, ctx.componentId);
            for (const cme::ConnectionData &edge : incoming)
            {
                output.output.insert(QString::fromStdString(edge.id()),
                                     ctx.incomingTokens.value(QString::fromStdString(edge.id())));
            }
        }
        output.output.insert(QStringLiteral("lastExecutedComponentId"), ctx.componentId);
    }

    return output;
}

bool SequentialExecutionEngine::validateExecutionResult(const ExecuteResult &result, QString &message)
{
    return cme::execution::validateExecutionResult(result, message);
}

void SequentialExecutionEngine::routeOutgoingTokens(const ExecutionContext &ctx, const ExecuteResult &result)
{
    const QList<cme::ConnectionData> outgoing = cme::helper::graph::getConnectionsBySourceId(m_graphSnapshot,
                                                                                                ctx.componentId);
    for (const cme::ConnectionData &edge : outgoing)
    {
        const QString connectionId = QString::fromStdString(edge.id());
        const QVariantMap payload = result.output;
        if (ctx.tokenRoutingEnabled)
        {
            cme::helper::graph::setPayload(m_graphSnapshot, connectionId, payload);
        }
        ++m_tokenWriteCount;
        const qint64 bytes = cme::helper::graph::estimatePayloadBytes(payload);
        m_payloadBytesWritten += bytes;
        m_maxPayloadBytes = qMax(m_maxPayloadBytes, bytes);
    }
}

void SequentialExecutionEngine::commitSchedulingState(const ExecutionContext &ctx)
{
    m_executed.insert(ctx.componentId);
    const QList<cme::ConnectionData> outgoing = cme::helper::graph::getConnectionsBySourceId(m_graphSnapshot,
                                                                                               ctx.componentId);
    for (const cme::ConnectionData &edge : outgoing)
    {
        const std::string targetId = edge.target_id();
        const int updatedInDegree = m_pendingInDegree.value(targetId, 0) - 1;
        m_pendingInDegree[targetId] = updatedInDegree;
        if (updatedInDegree == 0)
        {
            enqueueReadyComponent(QString::fromStdString(targetId));
        }
    }
}
