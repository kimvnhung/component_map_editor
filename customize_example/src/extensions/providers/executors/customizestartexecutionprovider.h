#ifndef CUSTOMIZESTARTEXECUTIONPROVIDER_H
#define CUSTOMIZESTARTEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeStartExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "start";

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

#endif // CUSTOMIZESTARTEXECUTIONPROVIDER_H
