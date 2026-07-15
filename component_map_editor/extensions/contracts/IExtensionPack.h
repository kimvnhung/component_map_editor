#ifndef IEXTENSIONPACK_H
#define IEXTENSIONPACK_H

#include <QString>

class ExtensionContractRegistry;

class IExtensionPack
{
public:
    virtual ~IExtensionPack() = default;

    // Registers all provider interfaces for this pack.
    // Manifest registration is handled by the startup loader.
    virtual bool registerProviders(ExtensionContractRegistry &registry, QString *error = nullptr) = 0;
};

#endif // IEXTENSIONPACK_H
