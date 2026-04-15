#ifndef CUSTOMIZELESSOREQUALEXECUTIONPROVIDER_H
#define CUSTOMIZELESSOREQUALEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeLessOrEqualExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "math/less_or_equal";

public:
    QString providerId() const override;
    QStringList supportedComponentTypes() const override;
    QStringList providedOutputKeys(const QString &componentType) const override;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot, const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload, QVariantMap *trace, QString *error) const override;
};

#endif // CUSTOMIZELESSOREQUALEXECUTIONPROVIDER_H
