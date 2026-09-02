#include "ExtensionContractRegistryBuilder.h"

ExtensionContractRegistryBuilder::ExtensionContractRegistryBuilder()
    : m_registry(std::make_unique<ExtensionContractRegistry>(ExtensionApiVersion{1, 0, 0}))
{}

std::unique_ptr<ExtensionContractRegistry> ExtensionContractRegistryBuilder::build()
{
    if (!m_registry)
    {
        throw std::runtime_error("ExtensionContractRegistryBuilder: registry is null.");
    }

    return std::move(m_registry);
}
