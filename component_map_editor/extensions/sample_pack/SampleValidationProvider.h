#ifndef SAMPLEVALIDATIONPROVIDER_H
#define SAMPLEVALIDATIONPROVIDER_H

#include "extensions/contracts/IValidationProvider.h"

// Sample implementation of IValidationProvider for the workflow domain.
// Validates a graphSnapshot QVariantMap with shape:
//   { components: [ { id, type, ... }, ... ],
//     connections: [ { id, sourceId, targetId, ... }, ... ] }
//
// Rules checked (codes used in tests):
//   W001  exactly one node of type "start" (error)
//   W002  exactly one node of type "end"   (error)
//   W003  all connection sourceId/targetId reference known component ids (error)
//   W004  no completely isolated nodes (nodes with neither incoming nor outgoing connections) (warning)
//
// Each issue QVariantMap has keys: code, severity, message, entityType, entityId.
class SampleValidationProvider : public IValidationProvider
{
public:
    QString      providerId() const override;
    QVariantList validateGraph(const QVariantMap &graphSnapshot) const override;
};

#endif // SAMPLEVALIDATIONPROVIDER_H
