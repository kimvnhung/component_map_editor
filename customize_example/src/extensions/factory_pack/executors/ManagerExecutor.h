#ifndef MANAGEREXECUTOR_H
#define MANAGEREXECUTOR_H

#include <extensions/contracts/IExecutionSemanticsProvider.h>

class ManagerExecutor : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override;
    QStringList supportedComponentTypes() const override;
    QStringList providedOutputKeys(const QString &componentType) const override;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot,
                          const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload,
                          QVariantMap *trace, QString *error) const override;
};

#endif // MANAGEREXECUTOR_H
