#ifndef CUSTOMIZEMULTIPLYEXECUTIONPROVIDER_H
#define CUSTOMIZEMULTIPLYEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeMultiplyExecutionProvider : public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "math/multiply";

    QString providerId() const override;
    QStringList supportedComponentTypes() const override;
    bool executeComponent(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &componentSnapshot,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override;
};

#endif // CUSTOMIZEMULTIPLYEXECUTIONPROVIDER_H
