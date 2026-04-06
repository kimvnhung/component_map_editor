#ifndef CUSTOMIZELOGICANDEXECUTIONPROVIDER_H
#define CUSTOMIZELOGICANDEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeLogicAndExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "logic/and";

    // IExecutionSemanticsProvider interface
public:
    QString providerId() const;
    QStringList supportedComponentTypes() const;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot, const QVariantMap &inputState, QVariantMap *outputState, QVariantMap *trace, QString *error) const;
    bool executeComponentV2(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot, const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload, QVariantMap *trace, QString *error) const;
};

#endif // CUSTOMIZELOGICANDEXECUTIONPROVIDER_H
