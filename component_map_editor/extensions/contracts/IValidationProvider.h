#ifndef IVALIDATIONPROVIDER_H
#define IVALIDATIONPROVIDER_H

#include <QString>
#include <QtPlugin>
#include <QVariantList>
#include <QVariantMap>

#include "adapters/ValidationAdapter.h"
#include "graph.pb.h"
#include "validation.pb.h"

class IValidationProvider
{
public:
    virtual ~IValidationProvider() = default;

    virtual QString providerId() const = 0;

    // Canonical typed contract.
    // Default implementation auto-bridges legacy map-based providers so
    // existing providers keep working during cutover.
    virtual bool validateGraph(const cme::GraphSnapshot &graphSnapshot,
                               cme::GraphValidationResult *outResult,
                               QString *error) const;

    // Legacy map-based contract kept as optional fallback during cutover.
    virtual QVariantList validateGraph(const QVariantMap &graphSnapshot) const;
};

#define COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER "ComponentMapEditor.Extensions.IValidationProvider/2.0"
Q_DECLARE_INTERFACE(IValidationProvider, COMPONENT_MAP_EDITOR_IID_VALIDATION_PROVIDER)

#endif // IVALIDATIONPROVIDER_H
