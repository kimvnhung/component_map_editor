#include "ruleruntimeregistrybuilder.h"

RuleRuntimeRegistryBuilder::RuleRuntimeRegistryBuilder() {}

RuleRuntimeRegistry *RuleRuntimeRegistryBuilder::build()
{
    return new RuleRuntimeRegistry();
}