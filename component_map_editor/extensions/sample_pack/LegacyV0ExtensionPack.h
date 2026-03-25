#ifndef LEGACYV0EXTENSIONPACK_H
#define LEGACYV0EXTENSIONPACK_H

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/ExtensionManifest.h"
#include "extensions/contracts/IExtensionPack.h"
#include "extensions/sample_pack/LegacyV0ExecutionSemanticsProvider.h"

class LegacyV0ExtensionPack : public IExtensionPack
{
public:
    static constexpr const char *ExtensionId = "sample.workflow.legacy-v0";

    LegacyV0ExtensionPack();

    const ExtensionManifest &manifest() const;
    bool registerAll(ExtensionContractRegistry &registry, QString *error = nullptr);

    bool registerProviders(ExtensionContractRegistry &registry, QString *error = nullptr) override;

private:
    ExtensionManifest m_manifest;
    LegacyV0ExecutionSemanticsProvider m_executionProvider;
};

#endif // LEGACYV0EXTENSIONPACK_H
