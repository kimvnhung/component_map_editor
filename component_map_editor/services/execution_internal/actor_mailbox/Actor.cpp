#include "Actor.h"

#include "adapters/GraphAdapter.h"
#include "services/ExecutionMigrationFlags.h"
#include "utils/GraphHelper.h"

namespace cme::actor
{

IActor::IActor(const QString &id, std::unique_ptr<IMailbox> mailbox)
    : m_id(id)
    , m_mailbox(std::move(mailbox))
{
}

bool IActor::enqueuedIfNot()
{
    bool expected = false;
    return m_enqueued.compare_exchange_strong(expected, true);
}

void IActor::markNotEnqueued()
{
    m_enqueued = false;
}

ComponentActor::ComponentActor(const QString &id,
                               const QString &componentType,
                               const cme::ComponentData &componentData,
                               const IExecutionSemanticsProvider *provider,
                               ActorFinishedCallback onFinished,
                               std::unique_ptr<IMailbox> mailbox)
    : IActor(id, std::move(mailbox))
    , m_componentType(componentType)
    , m_componentData(componentData)
    , m_provider(provider)
    , m_onFinished(std::move(onFinished))
{
}

void ComponentActor::onMessage(Message &&msg)
{
    ExecutionContext ctx;
    ctx.componentId = id();
    ctx.componentType = m_componentType;
    ctx.tokenRoutingEnabled = cme::execution::MigrationFlags::tokenTransportEnabled();
    ctx.incomingTokens = msg.incomingTokens;
    ctx.stepState = cme::helper::mergeIncomingTokens(ctx.incomingTokens);

    ExecuteResult result;
    result.componentData = m_componentData;

    if (m_provider)
    {
        QString error;
        result.providerId = m_provider->providerId();
        result.requiredOutputKeys = m_provider->providedOutputKeys(m_componentType);

        const QVariantMap componentSnapshot = cme::adapter::componentSnapshot(m_componentData);

        if (!m_provider->executeComponent(m_componentType, ctx.componentId, componentSnapshot,
                                          ctx.incomingTokens, &result.output, &result.trace, &error))
        {
            result.status = ExecuteResult::Status::Error;
            result.errorMessage = error;

            if (m_onFinished)
            {
                m_onFinished(ctx, result);
            }

            return;
        }

        result.status = ExecuteResult::Status::Ok;
    }
    else
    {
        result.trace.insert(QStringLiteral("provider"), QStringLiteral("default"));
        result.trace.insert(QStringLiteral("note"),
                            QStringLiteral("No execution semantics provider registered for component type."));

        if (ctx.tokenRoutingEnabled)
        {
            for (auto it = ctx.incomingTokens.constBegin(); it != ctx.incomingTokens.constEnd(); ++it)
            {
                result.output.insert(it.key(), it.value());
            }
        }

        result.output.insert(QStringLiteral("lastExecutedComponentId"), ctx.componentId);
        result.status = ExecuteResult::Status::Ok;
    }

    // Declared-output-key validation (cme::execution::validateExecutionResult) is deliberately NOT
    // run here - ActorMailboxExecutionEngine runs it once after popping a completed step, exactly
    // like SequentialExecutionEngine::executeNext(), so both engines distinguish ProviderError from
    // ValidationError identically instead of collapsing both into ExecuteResult::Status::Error here.
    if (m_onFinished)
    {
        m_onFinished(ctx, result);
    }
}

} // namespace cme::actor
