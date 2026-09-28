#include "SequentialExecutionEngine.h"

#include "services/ExecutionMigrationFlags.h"
#include "utils/GraphHelper.h"
#include "adapters/GraphAdapter.h"

SequentialExecutionEngine::SequentialExecutionEngine() {}


bool SequentialExecutionEngine::prepare(GraphModel *graph,
                                        const QHash<QString, const IExecutionSemanticsProvider *> &providersByType,
                                        const QVariantMap &inputSnapshot,
                                        QString *error)
{
    if (!graph)
    {
        if (error)
        {
            *error = QStringLiteral("Graph is not set.");
        }

        return false;
    }

    m_providerByComponentType = providersByType;
    m_inputSnapshot = inputSnapshot;

    const QList<ComponentModel *> components = graph->componentList();

    for (ComponentModel *component : components)
    {
        if (!component)
        {
            continue;
        }

        const QString componentId = component->id().trimmed();

        if (componentId.isEmpty())
        {
            continue;
        }

        cme::ComponentData *snap = m_graphSnapshot.add_components();
        snap->set_id(componentId.toStdString());
        snap->set_type_id(component->type().toStdString());
        snap->set_title(component->title().toStdString());
        // Set properties
        auto *properties = snap->mutable_properties();
        (*properties)["x"] = QString::number(component->x()).toStdString();
        (*properties)["y"] = QString::number(component->y()).toStdString();
        (*properties)["width"] = QString::number(component->width()).toStdString();
        (*properties)["height"] = QString::number(component->height()).toStdString();
        (*properties)["color"] = component->color().toStdString();
        (*properties)["shape"] = component->shape().toStdString();

        // Capture dynamic QML properties so extension execution semantics can
        // consume schema-defined fields (for example inputNumber/addValue).
        const QList<QByteArray> dynamicProps = component->dynamicPropertyNames();

        for (const QByteArray &propName : dynamicProps)
        {
            const QString key = QString::fromUtf8(propName);

            if (key.isEmpty())
            {
                continue;
            }

            (*properties)[key.toStdString()] = component->property(propName.constData()).toString().toStdString();
        }
    }

    for (auto it = m_graphSnapshot.components().begin(); it != m_graphSnapshot.components().end(); ++it)
    {
        m_pendingInDegree.insert(it->id(), 0);
    }

    const QList<ConnectionModel *> connections = graph->connectionList();

    for (ConnectionModel *connection : connections)
    {
        if (!connection)
        {
            continue;
        }

        cme::ConnectionData conn;
        conn.set_id(connection->id().toStdString());
        conn.set_source_id(connection->sourceId().toStdString());
        conn.set_target_id(connection->targetId().toStdString());
        conn.set_label(connection->label().toStdString());

        // Check if source and target components exist in the graph snapshot
        if (!m_pendingInDegree.contains(conn.source_id()) ||
                !m_pendingInDegree.contains(conn.target_id()))
        {
            if (error)
            {
                *error = QStringLiteral("Connection '%1' references non-existent source or target component.").arg(
                             QString::fromStdString(conn.id()));
            }

            return false;
        }

        m_pendingInDegree[conn.target_id()] = m_pendingInDegree.value(conn.target_id(), 0) + 1;
        m_graphSnapshot.mutable_connections()->Add(std::move(conn));
    }

    auto outgoingMap = cme::helper::getOutgoingConnectionsBySourceId(m_graphSnapshot);

    for (auto it = outgoingMap.begin(); it != outgoingMap.end(); ++it)
    {
        QList<cme::ConnectionData> &edges = it.value();
        std::sort(edges.begin(), edges.end(), [](const cme::ConnectionData & a, const cme::ConnectionData & b)
        {
            if (a.target_id() != b.target_id())
            {
                return a.target_id() < b.target_id();
            }

            return a.id() < b.id();
        });
    }

    auto incomingMap = cme::helper::getIncomingConnectionsByTargetId(m_graphSnapshot);

    for (auto it = incomingMap.begin(); it != incomingMap.end(); ++it)
    {
        QList<cme::ConnectionData> &edges = it.value();
        std::sort(edges.begin(), edges.end(), [](const cme::ConnectionData & a, const cme::ConnectionData & b)
        {
            if (a.source_id() != b.source_id())
            {
                return a.source_id() < b.source_id();
            }

            return a.id() < b.id();
        });
    }

    QStringList componentIds = cme::helper::getComponentIds(m_graphSnapshot);
    std::sort(componentIds.begin(), componentIds.end(), cme::helper::idComparator);

    for (const QString &componentId : componentIds)
    {
        if (m_pendingInDegree.value(componentId.toStdString(), 0) == 0)
        {
            enqueueReadyComponent(componentId);
        }
    }

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
    return cme::helper::getComponentCount(m_graphSnapshot);
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

    cme::ComponentData component = cme::helper::getComponentById(m_graphSnapshot, componentId);
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
    if (componentId.isEmpty() || m_executed.contains(componentId) || m_readyQueueSet.contains(componentId))
    {
        return;
    }

    auto it = std::lower_bound(m_readyQueue.begin(), m_readyQueue.end(), componentId, cme::helper::idComparator);
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

void SequentialExecutionEngine::prepareIncomingTokens(const QString &componentId, ExecutionContext &ctx,
        const QVariantMap &legacyGlobalState)
{
    const QList<cme::ConnectionData> incoming = cme::helper::getConnectionsByTargetId(m_graphSnapshot, componentId);

    const bool tokenRoutingEnabled = cme::execution::MigrationFlags::tokenTransportEnabled();

    if (tokenRoutingEnabled)
    {
        for (const cme::ConnectionData &edge : incoming)
        {
            auto tokenPayload = cme::helper::getConnectionPayloadById(m_graphSnapshot, QString::fromStdString(edge.id()));
            ctx.incomingTokens.insert(QString::fromStdString(edge.id()), tokenPayload);
        }

        if (ctx.incomingTokens.isEmpty() && !m_inputSnapshot.isEmpty())
        {
            ctx.incomingTokens.insert(QStringLiteral("__graph_input__"), m_inputSnapshot);
        }

        ctx.stepState = cme::helper::mergeIncomingTokens(ctx.incomingTokens);
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
        const qint64 bytes = cme::helper::estimatePayloadBytes(ctx.incomingTokens.value(tokenId));
        m_payloadBytesRead += bytes;
        m_maxPayloadBytes = qMax(m_maxPayloadBytes, bytes);
    }
}


ExecuteResult SequentialExecutionEngine::invokeProvider(const ExecutionContext &ctx)
{
    ExecuteResult output;
    output.componentData = cme::helper::getComponentById(m_graphSnapshot,
                           ctx.componentId);
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
            QList<cme::ConnectionData> incoming = cme::helper::getConnectionsByTargetId(m_graphSnapshot, ctx.componentId);

            for (const cme::ConnectionData &edge : incoming)
            {
                output.output.insert(QString::fromStdString(edge.id()), ctx.incomingTokens.value(QString::fromStdString(edge.id())));
            }
        }

        output.output.insert(QStringLiteral("lastExecutedComponentId"), ctx.componentId);
    }

    return output;
}


bool SequentialExecutionEngine::validateExecutionResult(const ExecuteResult &result, QString &message)
{
    // Validate that outputPayload contains at least one of the provider's
    // declared output keys. Also accept any keys explicitly configured
    // in the component's snapshot (e.g. outputKey="mySum"), since those
    // override the default declared names.
    const QStringList declared = result.requiredOutputKeys;

    if (!declared.isEmpty())
    {
        bool matched = false;

        for (const QString &key : declared)
        {
            if (result.output.contains(key))
            {
                matched = true;
                break;
            }
        }

        if (!matched)
        {
            // Fall back: check any snapshot-level output-key property
            static const QStringList kOutputKeyProps =
            {
                QStringLiteral("outputKey"),
                QStringLiteral("trueRouteKey"),
                QStringLiteral("falseRouteKey"),
                QStringLiteral("iterKey"),
                QStringLiteral("continueKey"),
                QStringLiteral("errorKey")
            };

            const auto& properties = result.componentData.properties();

            for (const QString &prop : kOutputKeyProps)
            {
                auto it = properties.find(prop.toStdString());

                if (it != properties.end())
                {
                    const QString configured = QString::fromStdString(it->second);

                    if (!configured.isEmpty() && result.output.contains(configured))
                    {
                        matched = true;
                        break;
                    }
                }
            }
        }

        if (!matched)
        {
            // markError(QStringLiteral("Provider '%1': output payload for type '%2' is missing all declared keys [%3].")
            //           .arg(result.providerId.toStdString(), resu, declared.join(QStringLiteral(", "))));
            // return false;
            message = QString("Provider '%1': output payload for type '%2' is missing all declared keys [%3].")
                      .arg(result.providerId, result.componentData.type_id().c_str(), declared.join(QStringLiteral(", ")));
        }

        return matched;
    }

    return true;
}


void SequentialExecutionEngine::routeOutgoingTokens(const ExecutionContext& ctx, const ExecuteResult &result)
{
    const QList<cme::ConnectionData> outgoing = cme::helper::getConnectionsBySourceId(m_graphSnapshot, ctx.componentId);

    // Route tokens to outgoing connections
    for (const cme::ConnectionData &edge : outgoing)
    {
        const QString connectionId = QString::fromStdString(edge.id());
        const QVariantMap payload = result.output;

        if (ctx.tokenRoutingEnabled)
        {
            // Store the payload in the graph snapshot for the outgoing connection
            cme::helper::setPayload(m_graphSnapshot, connectionId, payload);
        }

        ++m_tokenWriteCount;
        const qint64 bytes = cme::helper::estimatePayloadBytes(payload);
        m_payloadBytesWritten += bytes;
        m_maxPayloadBytes = qMax(m_maxPayloadBytes, bytes);
    }

}


void SequentialExecutionEngine::commitSchedulingState(const ExecutionContext &ctx)
{
    m_executed.insert(ctx.componentId);
    const QList<cme::ConnectionData> outgoing = cme::helper::getConnectionsBySourceId(m_graphSnapshot, ctx.componentId);

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
