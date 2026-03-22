#ifndef IEXECUTIONSEMANTICSPROVIDERV0_H
#define IEXECUTIONSEMANTICSPROVIDERV0_H

#include <QString>
#include <QStringList>
#include <QtPlugin>
#include <QVariantMap>

// Legacy execution semantics contract (v0).
// This previous contract version did not support a dedicated trace map output.
class IExecutionSemanticsProviderV0
{
public:
    virtual ~IExecutionSemanticsProviderV0() = default;

    virtual QString providerId() const = 0;
    virtual QStringList supportedComponentTypes() const = 0;

    virtual bool executeComponent(const QString &componentType,
                                  const QString &componentId,
                                  const QVariantMap &componentSnapshot,
                                  const QVariantMap &inputState,
                                  QVariantMap *outputState,
                                  QString *error) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_EXECUTION_SEMANTICS_PROVIDER_V0 "ComponentMapEditor.Extensions.IExecutionSemanticsProvider/0.9"
Q_DECLARE_INTERFACE(IExecutionSemanticsProviderV0,
                    COMPONENT_MAP_EDITOR_IID_EXECUTION_SEMANTICS_PROVIDER_V0)

#endif // IEXECUTIONSEMANTICSPROVIDERV0_H
