#ifndef CUSTOMIZELESSTHANEXECUTIONPROVIDER_H
#define CUSTOMIZELESSTHANEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeLessThanExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "math/less_than";

    // IExecutionSemanticsProvider interface
public:
    QString providerId() const;
    QStringList supportedComponentTypes() const;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot, const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload, QVariantMap *trace, QString *error) const;
};

#endif // CUSTOMIZELESSTHANEXECUTIONPROVIDER_H
