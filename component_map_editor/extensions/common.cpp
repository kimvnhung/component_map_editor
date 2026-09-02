#include "common.h"

namespace extensions
{
    QString capabilityToString(Capability capability)
    {
        {
            switch (capability)
            {
                case Capability_ComponentTypes: return QStringLiteral("componentTypes");

                case Capability_ConnectionPolicy: return QStringLiteral("connectionPolicy");

                case Capability_PropertySchema: return QStringLiteral("propertySchema");

                case Capability_Validation: return QStringLiteral("validation");

                case Capability_Actions: return QStringLiteral("actions");

                case Capability_ExecutionSemantics: return QStringLiteral("executionSemantics");

                default: return QStringLiteral("unknown");
            }
        }
    }

    Capability capabilityFromString(const QString &capabilityStr)
    {
        if (capabilityStr == QStringLiteral("componentTypes"))
        {
            return Capability_ComponentTypes;
        }
        else if (capabilityStr == QStringLiteral("connectionPolicy"))
        {
            return Capability_ConnectionPolicy;
        }
        else if (capabilityStr == QStringLiteral("propertySchema"))
        {
            return Capability_PropertySchema;
        }
        else if (capabilityStr == QStringLiteral("validation"))
        {
            return Capability_Validation;
        }
        else if (capabilityStr == QStringLiteral("actions"))
        {
            return Capability_Actions;
        }
        else if (capabilityStr == QStringLiteral("executionSemantics"))
        {
            return Capability_ExecutionSemantics;
        }

        return Capability_None;
    }

    Capability capabilitiesFromArray(const QStringList &capabilityStrList)
    {
        Capability capabilities = Capability_None;

        for (const QString &capabilityStr : capabilityStrList)
        {
            capabilities = static_cast<Capability>(capabilities | capabilityFromString(capabilityStr));
        }

        return capabilities;
    }

    bool hasCapability(const Capability &capability, Capability capabilities)
    {
        return (capabilities & capability) == capability;
    }
}