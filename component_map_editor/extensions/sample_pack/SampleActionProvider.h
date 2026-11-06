#ifndef SAMPLEACTIONPROVIDER_H
#define SAMPLEACTIONPROVIDER_H

#include "extensions/contracts/IActionProvider.h"

// Sample implementation of IActionProvider for the workflow domain.
// The provider exposes action metadata for presentation in the UI.
// Graph mutations are performed only by user-facing UI workflows.
class SampleActionProvider : public IActionProvider
{
public:
    static constexpr const char *ActionSetTaskPriority = "setTaskPriority";

    QString     providerId() const override;
    QStringList actionIds() const override;
    QVariantMap actionDescriptor(const QString &actionId) const override;
};

#endif // SAMPLEACTIONPROVIDER_H
