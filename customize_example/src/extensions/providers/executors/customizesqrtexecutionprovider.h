#ifndef CUSTOMIZESQRTEXECUTIONPROVIDER_H
#define CUSTOMIZESQRTEXECUTIONPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProvider.h"

class CustomizeSqrtExecutionProvider : public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "math/sqrt";

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

#endif // CUSTOMIZESQRTEXECUTIONPROVIDER_H