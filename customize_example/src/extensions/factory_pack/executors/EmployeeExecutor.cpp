#include "EmployeeExecutor.h"

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"

QString EmployeeExecutor::providerId() const
{
    return QStringLiteral("factory.execution.employee");
}

QStringList EmployeeExecutor::supportedComponentTypes() const
{
    return { FactoryComponentTypeProvider::TypeEmployee };
}

QStringList EmployeeExecutor::providedOutputKeys(const QString &componentType) const
{
    Q_UNUSED(componentType);
    return { QStringLiteral("washed"), QStringLiteral("fruit_type"), QStringLiteral("request_wash"),
             QStringLiteral("revenue"), QStringLiteral("request_sell") };
}

bool EmployeeExecutor::executeComponent(const QString &componentType, const QString &componentId,
                                        const QVariantMap &componentSnapshot,
                                        const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload,
                                        QVariantMap *trace, QString *error) const
{
    Q_UNUSED(componentType);
    Q_UNUSED(componentId);
    Q_UNUSED(componentSnapshot);
    Q_UNUSED(incomingTokens);
    Q_UNUSED(trace);
    Q_UNUSED(error);

    if (outputPayload)
    {
        // For demonstration purposes, we simulate an employee that can wash and sell fruit.
        (*outputPayload)[QStringLiteral("washed")] = 5; // Wash 5 units of fruit
        (*outputPayload)[QStringLiteral("fruit_type")] = QStringLiteral("apple"); // Fruit type is apple
        (*outputPayload)[QStringLiteral("request_wash")] = QStringLiteral("store_1"); // Request wash from store_1
        (*outputPayload)[QStringLiteral("revenue")] = 15.0; // Revenue from selling fruit
        (*outputPayload)[QStringLiteral("request_sell")] = QStringLiteral("store_1"); // Request sell from store_1
    }

    return true;
}