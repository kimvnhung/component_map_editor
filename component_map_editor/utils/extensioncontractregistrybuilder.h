#ifndef EXTENSIONCONTRACTREGISTRYBUILDER_H
#define EXTENSIONCONTRACTREGISTRYBUILDER_H

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"


class ExtensionContractRegistryBuilder
{
public:
    ExtensionContractRegistryBuilder();

    ExtensionContractRegistry *build();
private:
    RuleBackedConnectionPolicyProvider *m_connectionPolicyProvider = nullptr;

};

#endif // EXTENSIONCONTRACTREGISTRYBUILDER_H
