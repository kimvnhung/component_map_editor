#ifndef SAMPLECOMPONENTTYPEPROVIDER_H
#define SAMPLECOMPONENTTYPEPROVIDER_H

#include "extensions/contracts/IComponentTypeProvider.h"

// Sample implementation of IComponentTypeProvider for a simple workflow domain.
// Declares four component types: start, task, decision, end.
// Used as a reference implementation and contract test fixture.
class SampleComponentTypeProvider : public IComponentTypeProvider
{
public:
    static constexpr const char *TypeStart    = "start";
    static constexpr const char *TypeTask     = "task";
    static constexpr const char *TypeDecision = "decision";
    static constexpr const char *TypeEnd      = "end";

    QString      providerId() const override;
    QStringList  componentTypeIds() const override;

    // Descriptor keys: id, title, category, defaultWidth, defaultHeight,
    //                  defaultColor, allowIncoming, allowOutgoing.
    QVariantMap  componentTypeDescriptor(const QString &componentTypeId) const override;

    // Default property keys for each type:
    //   task     -> priority("normal"), description("")
    //   decision -> condition("")
    //   start, end -> (empty defaults)
    QVariantMap  defaultComponentProperties(const QString &componentTypeId) const override;
};

#endif // SAMPLECOMPONENTTYPEPROVIDER_H