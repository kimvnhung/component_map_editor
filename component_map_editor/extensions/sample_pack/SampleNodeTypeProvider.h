#ifndef SAMPLENODETYPEPROVIDER_H
#define SAMPLENODETYPEPROVIDER_H

#include "extensions/contracts/INodeTypeProvider.h"

// Sample implementation of INodeTypeProvider for a simple workflow domain.
// Declares four node types: start, task, decision, end.
// Used as a reference implementation and contract test fixture.
class SampleNodeTypeProvider : public INodeTypeProvider
{
public:
    // Declared type IDs (use these constants in tests to avoid magic strings).
    static constexpr const char *TypeStart    = "start";
    static constexpr const char *TypeTask     = "task";
    static constexpr const char *TypeDecision = "decision";
    static constexpr const char *TypeEnd      = "end";

    QString      providerId() const override;
    QStringList  nodeTypeIds() const override;

    // Descriptor keys: id, title, category, defaultWidth, defaultHeight,
    //                  defaultColor, allowIncoming, allowOutgoing.
    QVariantMap  nodeTypeDescriptor(const QString &nodeTypeId) const override;

    // Default property keys for each type:
    //   task     -> priority("normal"), description("")
    //   decision -> condition("")
    //   start, end -> (empty defaults)
    QVariantMap  defaultNodeProperties(const QString &nodeTypeId) const override;
};

#endif // SAMPLENODETYPEPROVIDER_H
