#ifndef EXTENSIONCONTRACTREGISTRYBUILDER_H
#define EXTENSIONCONTRACTREGISTRYBUILDER_H

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"


class ExtensionContractRegistryBuilder
{
public:
    ExtensionContractRegistryBuilder();

    std::unique_ptr<ExtensionContractRegistry> build();
private:
    RuleBackedConnectionPolicyProvider *m_connectionPolicyProvider = nullptr;

    std::unique_ptr<ExtensionContractRegistry> m_registry{nullptr};
};

#endif // EXTENSIONCONTRACTREGISTRYBUILDER_H
