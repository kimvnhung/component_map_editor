#ifndef SAMPLEEXTENSIONPACK_H
#define SAMPLEEXTENSIONPACK_H

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/ExtensionManifest.h"
#include "extensions/contracts/IExtensionPack.h"

#include "SampleActionProvider.h"
#include "SampleConnectionPolicyProvider.h"
#include "SampleComponentTypeProvider.h"
#include "SamplePropertySchemaProvider.h"
#include "SampleValidationProvider.h"

// SampleExtensionPack aggregates all five sample providers and exposes a
// single registerAll() entry point.  This is the canonical usage pattern
// that all real business packs must follow.
//
// The pack owns all provider instances; the registry stores non-owning
// pointers and must not outlive the pack.
class SampleExtensionPack : public IExtensionPack
{
public:
    static constexpr const char *ExtensionId      = "sample.workflow";
    static constexpr const char *DisplayName      = "Sample Workflow Pack";
    static constexpr const char *ExtensionVersion = "1.0.0";

    // Core API compatibility range declared by this pack.
    static constexpr int MinApiMajor = 1;
    static constexpr int MinApiMinor = 0;
    static constexpr int MinApiPatch = 0;
    static constexpr int MaxApiMajor = 1;
    static constexpr int MaxApiMinor = 99;
    static constexpr int MaxApiPatch = 99;

    SampleExtensionPack();

    const ExtensionManifest &manifest() const;

    // Registers the manifest and all five providers with the given registry.
    // Returns false and populates error on the first failure encountered.
    bool registerProviders(ExtensionContractRegistry &registry, QString *error = nullptr) override;
    bool registerAll(ExtensionContractRegistry &registry, QString *error = nullptr);

private:
    ExtensionManifest         m_manifest;
    SampleComponentTypeProvider m_componentTypeProvider;
    SampleConnectionPolicyProvider m_connectionPolicyProvider;
    SamplePropertySchemaProvider   m_propertySchemaProvider;
    SampleValidationProvider  m_validationProvider;
    SampleActionProvider      m_actionProvider;
};

#endif // SAMPLEEXTENSIONPACK_H
