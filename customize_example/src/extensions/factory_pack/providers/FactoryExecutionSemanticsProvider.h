#ifndef FACTORYEXECUTIONSEMANTICSPROVIDER_H
#define FACTORYEXECUTIONSEMANTICSPROVIDER_H

#include <extensions/contracts/IExecutionSemanticsProvider.h>

class FactoryExecutionSemanticsProvider : public IExecutionSemanticsProvider
{
public:
    FactoryExecutionSemanticsProvider();

    // IExecutionSemanticsProvider interface
public:
    QString providerId() const override;
    QStringList supportedComponentTypes() const override;
    QStringList providedOutputKeys(const QString &componentType) const override;
    bool executeComponent(const QString &componentType, const QString &componentId, const QVariantMap &componentSnapshot,
                          const cme::execution::IncomingTokens &incomingTokens, cme::execution::ExecutionPayload *outputPayload,
                          QVariantMap *trace, QString *error) const override;
private:
    QList<IExecutionSemanticsProvider *> m_providers;
};

#endif // FACTORYEXECUTIONSEMANTICSPROVIDER_H
