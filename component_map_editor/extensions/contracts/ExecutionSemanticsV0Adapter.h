#ifndef EXECUTIONSEMANTICSV0ADAPTER_H
#define EXECUTIONSEMANTICSV0ADAPTER_H

#include "IExecutionSemanticsProvider.h"
#include "IExecutionSemanticsProviderV0.h"

class ExecutionSemanticsV0Adapter : public IExecutionSemanticsProvider
{
public:
    explicit ExecutionSemanticsV0Adapter(const IExecutionSemanticsProviderV0 *legacyProvider);

    QString providerId() const override;
    QStringList supportedComponentTypes() const override;

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QVariantMap *trace,
                          QString *error) const override;

private:
    const IExecutionSemanticsProviderV0 *m_legacyProvider = nullptr;
};

#endif // EXECUTIONSEMANTICSV0ADAPTER_H
