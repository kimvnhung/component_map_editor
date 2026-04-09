#ifndef CUSTOMIZELESSOREQUALEXECUTIONPROVIDER_H
#define CUSTOMIZELESSOREQUALEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeLessOrEqualExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "math/less_or_equal";

public:
    QString providerId() const;
    QStringList supportedComponentTypes() const;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot, const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload, QVariantMap *trace, QString *error) const;
};

#endif // CUSTOMIZELESSOREQUALEXECUTIONPROVIDER_H
