#include "FactoryExecutionSemanticsProvider.h"

#include "extensions/factory_pack/executors/EmployeeExecutor.h"
#include "extensions/factory_pack/executors/FruitProducerExecutor.h"
#include "extensions/factory_pack/executors/ManagerExecutor.h"
#include "extensions/factory_pack/executors/SellerExecutor.h"
#include "extensions/factory_pack/executors/StoreExecutor.h"

FactoryExecutionSemanticsProvider::FactoryExecutionSemanticsProvider()
{
    m_providers.append(new FruitProducerExecutor());
    m_providers.append(new StoreExecutor());
    m_providers.append(new EmployeeExecutor());
    m_providers.append(new SellerExecutor());
    m_providers.append(new ManagerExecutor());
}

QString FactoryExecutionSemanticsProvider::providerId() const
{
    return QStringLiteral("factory.execution");
}

QStringList FactoryExecutionSemanticsProvider::supportedComponentTypes() const
{
    QStringList types;

    for (const IExecutionSemanticsProvider *provider : m_providers)
    {
        types.append(provider->supportedComponentTypes());
    }

    return types;
}

QStringList FactoryExecutionSemanticsProvider::providedOutputKeys(const QString &componentType) const
{
    for (const IExecutionSemanticsProvider *provider : m_providers)
    {
        if (provider->supportedComponentTypes().contains(componentType))
        {
            return provider->providedOutputKeys(componentType);
        }
    }

    return {};
}

bool FactoryExecutionSemanticsProvider::executeComponent(const QString &componentType, const QString &componentId,
        const QVariantMap &componentSnapshot,
        const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload,
        QVariantMap *trace, QString *error) const
{
    for (const IExecutionSemanticsProvider *provider : m_providers)
    {
        if (provider->supportedComponentTypes().contains(componentType))
        {
            return provider->executeComponent(componentType, componentId, componentSnapshot, incomingTokens, outputPayload, trace,
                                              error);
        }
    }

    if (error)
    {
        *error = QStringLiteral("No execution semantics provider registered for component type '%1'.").arg(componentType);
    }

    return false;
}