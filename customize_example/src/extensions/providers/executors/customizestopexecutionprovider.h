#ifndef CUSTOMIZESTOPEXECUTIONPROVIDER_H
#define CUSTOMIZESTOPEXECUTIONPROVIDER_H

#include <extensions/contracts/IExecutionSemanticsProvider.h>

class CustomizeStopExecutionProvider: public IExecutionSemanticsProvider
{
public:
    static constexpr const char *TypeId = "stop";

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

#endif // CUSTOMIZESTOPEXECUTIONPROVIDER_H
