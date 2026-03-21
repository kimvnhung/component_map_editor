#ifndef IEXECUTIONSEMANTICSPROVIDER_H
#define IEXECUTIONSEMANTICSPROVIDER_H

#include <QString>
#include <QStringList>
#include <QtPlugin>
#include <QVariantMap>

class IExecutionSemanticsProvider
{
public:
    virtual ~IExecutionSemanticsProvider() = default;

    virtual QString providerId() const = 0;
    virtual QStringList supportedComponentTypes() const = 0;

    // Executes one component in sandbox context.
    // Implementations must be side-effect free against live graph/editor state.
    virtual bool executeComponent(const QString &componentType,
                                  const QString &componentId,
                                  const QVariantMap &componentSnapshot,
                                  const QVariantMap &inputState,
                                  QVariantMap *outputState,
                                  QVariantMap *trace,
                                  QString *error) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_EXECUTION_SEMANTICS_PROVIDER "ComponentMapEditor.Extensions.IExecutionSemanticsProvider/1.0"
Q_DECLARE_INTERFACE(IExecutionSemanticsProvider, COMPONENT_MAP_EDITOR_IID_EXECUTION_SEMANTICS_PROVIDER)

#endif // IEXECUTIONSEMANTICSPROVIDER_H
