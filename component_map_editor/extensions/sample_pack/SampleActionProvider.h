#ifndef SAMPLEACTIONPROVIDER_H
#define SAMPLEACTIONPROVIDER_H

#include "extensions/contracts/IActionProvider.h"

// Sample implementation of IActionProvider for the workflow domain.
// Registered actions:
//   setTaskPriority  – changes the priority property of a task node.
//
// invokeAction context keys for setTaskPriority:
//   componentId  (required) – id of the target task node
//   newPriority  (required) – one of: "low", "normal", "high"
//
// Produced commandRequest keys (consumed by core command gateway):
//   commandType   = "setComponentProperty"
//   componentId   = <from context>
//   propertyName  = "priority"
//   newValue      = <from context newPriority>
class SampleActionProvider : public IActionProvider
{
public:
    static constexpr const char *ActionSetTaskPriority = "setTaskPriority";

    QString     providerId() const override;
    QStringList actionIds() const override;
    QVariantMap actionDescriptor(const QString &actionId) const override;
    bool        invokeAction(const QString &actionId,
                             const QVariantMap &context,
                             QVariantMap *commandRequest,
                             QString *error) const override;
};

#endif // SAMPLEACTIONPROVIDER_H
