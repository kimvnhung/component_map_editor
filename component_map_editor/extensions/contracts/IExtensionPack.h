#ifndef IEXTENSIONPACK_H
#define IEXTENSIONPACK_H

#include <QString>

#include "extensions/runtime/PropertySchemaRegistry.h"
#include "services/GraphExecutionSandbox.h"

class ExtensionContractRegistry;

class IExtensionPack
{
public:
    virtual ~IExtensionPack() = default;

    // Registers all provider interfaces for this pack.
    // Manifest registration is handled by the startup loader.
    virtual bool registerProviders(ExtensionContractRegistry &registry, QString *error = nullptr) = 0;

    // Optional hook for packs created via ExtensionPackBuilder to provide
    // auxiliary registries (PropertySchemaRegistry, GraphExecutionSandbox)
    // that should replace the application's defaults. Default no-op.
    virtual void rebuildAuxiliaryRegistries(std::unique_ptr<PropertySchemaRegistry> *outPropRegistry,
                                           std::unique_ptr<GraphExecutionSandbox> *outSandbox) const
    {
        if (outPropRegistry) *outPropRegistry = nullptr;
        if (outSandbox) *outSandbox = nullptr;
    }
};

#endif // IEXTENSIONPACK_H
