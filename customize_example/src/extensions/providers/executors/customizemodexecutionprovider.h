#ifndef CUSTOMIZEMODEXECUTIONPROVIDER_H
#define CUSTOMIZEMODEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeModExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "math/mod";

    QString providerId() const;
    QStringList supportedComponentTypes() const;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot, const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload, QVariantMap *trace, QString *error) const;
};

#endif // CUSTOMIZEMODEXECUTIONPROVIDER_H
