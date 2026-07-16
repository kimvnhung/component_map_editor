#ifndef RULERUNTIMEREGISTRYBUILDER_H
#define RULERUNTIMEREGISTRYBUILDER_H

#include "extensions/runtime/rules/RuleRuntimeRegistry.h"

class RuleRuntimeRegistryBuilder
{
public:
    RuleRuntimeRegistryBuilder();
    RuleRuntimeRegistry *build();
};

#endif // RULERUNTIMEREGISTRYBUILDER_H
