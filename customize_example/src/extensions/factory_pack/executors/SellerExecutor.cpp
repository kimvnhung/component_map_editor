#include "SellerExecutor.h"

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"

QString SellerExecutor::providerId() const
{
    return QStringLiteral("factory.execution.seller");
}

QStringList SellerExecutor::supportedComponentTypes() const
{
    return { FactoryComponentTypeProvider::TypeSeller };
}


QStringList SellerExecutor::providedOutputKeys(const QString &componentType) const
{
    Q_UNUSED(componentType);
    return { QStringLiteral("sold"), QStringLiteral("fruit_type"), QStringLiteral("revenue") };
}

bool SellerExecutor::executeComponent(const QString &componentType, const QString &componentId,
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
        // For demonstration purposes, we simulate a seller that can sell fruit.
        (*outputPayload)[QStringLiteral("sold")] = 3; // Sell 3 units of fruit
        (*outputPayload)[QStringLiteral("fruit_type")] = QStringLiteral("apple"); // Fruit type is apple
        (*outputPayload)[QStringLiteral("revenue")] = 15.0; // Revenue from selling fruit
    }

    return true;
}