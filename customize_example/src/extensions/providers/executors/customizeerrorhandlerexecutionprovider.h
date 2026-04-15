#ifndef CUSTOMIZEERRORHANDLEREXECUTIONPROVIDER_H
#define CUSTOMIZEERRORHANDLEREXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeErrorHandlerExecutionProvider : public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "system/error_handler";

    QString providerId() const override;
    QStringList supportedComponentTypes() const override;
    QStringList providedOutputKeys(const QString &componentType) const override;
    bool executeComponent(const QString &componentType,
                            const QString &componentId,
                            const QVariantMap &componentSnapshot,
                            const cme::execution::IncomingTokens &incomingTokens,
                            cme::execution::ExecutionPayload *outputPayload,
                            QVariantMap *trace,
                            QString *error) const override;
};

#endif // CUSTOMIZEERRORHANDLEREXECUTIONPROVIDER_H
