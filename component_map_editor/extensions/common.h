#ifndef EXTENSIONS_COMMON_H
#define EXTENSIONS_COMMON_H

#include <QStringList>

namespace extensions
{
    enum Capability
    {
        Capability_None = 0x000000,
        Capability_ComponentTypes = 0x000001,
        Capability_ConnectionPolicy = 0x000010,
        Capability_PropertySchema = 0x000100,
        Capability_Validation = 0x001000,
        Capability_Actions = 0x010000,
        Capability_ExecutionSemantics = 0x100000,
        // All capabilities, include ComponentTypes, ConnectionPolicy, PropertySchema, Validation, Actions, and ExecutionSemantics
        Capability_All = 0x111111
    };

    QString capabilityToString(Capability capability);
    Capability capabilityFromString(const QString &capabilityStr);

    Capability capabilitiesFromArray(const QStringList &capabilityStrList);

    bool hasCapability(const Capability &capability, Capability capabilities);
}

#endif // EXTENSIONS_COMMON_H
