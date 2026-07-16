#include "extensioncontractregistrybuilder.h"

ExtensionContractRegistryBuilder::ExtensionContractRegistryBuilder()
{}

ExtensionContractRegistry *ExtensionContractRegistryBuilder::build()
{
    return new ExtensionContractRegistry({1, 0, 0});
}