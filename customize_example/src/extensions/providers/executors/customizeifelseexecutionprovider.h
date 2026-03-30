#ifndef CUSTOMIZEIFELSEEXECUTIONPROVIDER_H
#define CUSTOMIZEIFELSEEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeIfElseExecutionProvider : public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "control/ifelse";

    QString providerId() const override;
    QStringList supportedComponentTypes() const override;

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QVariantMap *trace,
                          QString *error) const override;

    bool executeComponentV2(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &componentSnapshot,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override;
};

#endif // CUSTOMIZEIFELSEEXECUTIONPROVIDER_H
