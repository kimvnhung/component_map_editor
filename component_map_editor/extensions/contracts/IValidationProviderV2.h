#ifndef IVALIDATIONPROVIDERV2_H
#define IVALIDATIONPROVIDERV2_H

#include <QString>
#include <QtPlugin>

#include "graph.pb.h"
#include "validation.pb.h"

class IValidationProviderV2
{
public:
    virtual ~IValidationProviderV2() = default;

    virtual QString providerId() const = 0;

    // Typed validation contract for Phase 7.
    // Returns true on successful provider execution. Logical validation failures
    // should be expressed via issues in outResult, not by returning false.
    virtual bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                               cme::GraphValidationResult *outResult,
                               QString *error) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER_V2 "ComponentMapEditor.Extensions.IValidationProviderV2/2.0"
Q_DECLARE_INTERFACE(IValidationProviderV2, COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER_V2)

#endif // IVALIDATIONPROVIDERV2_H
