#ifndef CUSTOMIZEEXTENSIONPACK_H
#define CUSTOMIZEEXTENSIONPACK_H


#include <QObject>

#include <extensions/contracts/ExtensionManifest.h>
#include <extensions/contracts/IExtensionPack.h>

#include "customizecomponenttypeprovider.h"
#include "customizeconnectionpolicyprovider.h"
#include "customizepropertyschemaprovider.h"

// CustomizeExtensionPack aggregates sample providers and exposes a
// single registerAll() entry point.  This is the canonical usage pattern
// that all real business packs must follow.
//
// The pack owns all provider instances; the registry stores non-owning
// pointers and must not outlive the pack.
class CustomizeExtensionPack : public IExtensionPack
{
public:
    static constexpr const char *ExtensionId      = "customize.workflow";
    static constexpr const char *DisplayName      = "Customize Workflow Pack";
    static constexpr const char *ExtensionVersion = "1.0.0";

    // Core API compatibility range declared by this pack.
    static constexpr int MinApiMajor = 1;
    static constexpr int MinApiMinor = 0;
    static constexpr int MinApiPatch = 0;
    static constexpr int MaxApiMajor = 1;
    static constexpr int MaxApiMinor = 99;
    static constexpr int MaxApiPatch = 99;

    CustomizeExtensionPack();

    const ExtensionManifest &manifest() const;

    // Registers the manifest and all providers with the given registry.
    // Returns false and populates error on the first failure encountered.
    bool registerProviders(ExtensionContractRegistry &registry, QString *error = nullptr) override;
    bool registerAll(ExtensionContractRegistry &registry, QString *error = nullptr);

private:
    ExtensionManifest         m_manifest;
    CustomizeComponentTypeProvider m_componentTypeProvider;
    CustomizeConnectionPolicyProvider m_connectionPolicyProvider;
    CustomizePropertySchemaProvider   m_propertySchemaProvider;
    // WorkflowValidationProvider m_workflowValidationProvider;
    // SampleActionProvider      m_actionProvider;
    // SampleExecutionSemanticsProvider m_executionSemanticsProvider;
};

#endif // CUSTOMIZEEXTENSIONPACK_H
