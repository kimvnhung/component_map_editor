#ifndef SAMPLEVALIDATIONPROVIDER_H
#define SAMPLEVALIDATIONPROVIDER_H

#include "built_in_validation_providers/WorkflowValidationProvider.h"

// Compatibility alias class.
// New code should depend on WorkflowValidationProvider directly.
class SampleValidationProvider : public WorkflowValidationProvider
{
};

#endif // SAMPLEVALIDATIONPROVIDER_H
