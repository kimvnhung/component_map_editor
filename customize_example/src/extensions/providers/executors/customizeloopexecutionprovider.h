#ifndef CUSTOMIZELOOPEXECUTIONPROVIDER_H
#define CUSTOMIZELOOPEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeLoopExecutionProvider : public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "control/loop";

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

#endif // CUSTOMIZELOOPEXECUTIONPROVIDER_H
