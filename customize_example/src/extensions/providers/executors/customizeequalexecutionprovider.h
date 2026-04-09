#ifndef CUSTOMIZEEQUALEXECUTIONPROVIDER_H
#define CUSTOMIZEEQUALEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeEqualExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "math/equal";


    // IExecutionSemanticsProvider interface
public:
    QString providerId() const;
    QStringList supportedComponentTypes() const;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot, const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload, QVariantMap *trace, QString *error) const;
};

#endif // CUSTOMIZEEQUALEXECUTIONPROVIDER_H
