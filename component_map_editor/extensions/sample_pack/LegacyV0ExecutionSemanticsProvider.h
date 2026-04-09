#ifndef LEGACYV0EXECUTIONSEMANTICSPROVIDER_H
#define LEGACYV0EXECUTIONSEMANTICSPROVIDER_H

#include "extensions/contracts/IExecutionSemanticsProviderV0.h"

class LegacyV0ExecutionSemanticsProvider : public IExecutionSemanticsProviderV0
{
public:
    QString providerId() const override;
    QStringList supportedComponentTypes() const override;

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &inputState,
                          QVariantMap *outputState,
                          QString *error) const override;
};

#endif // LEGACYV0EXECUTIONSEMANTICSPROVIDER_H
