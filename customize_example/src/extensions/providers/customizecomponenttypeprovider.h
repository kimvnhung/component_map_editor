#ifndef CUSTOMIZECOMPONENTTYPEPROVIDER_H
#define CUSTOMIZECOMPONENTTYPEPROVIDER_H

#include "extensions/contracts/IComponentTypeProvider.h"

// Sample implementation of IComponentTypeProvider for a simple workflow domain.
// Declares three component types: start, process, stop.
// Used as a reference implementation and contract test fixture.
class CustomizeComponentTypeProvider : public IComponentTypeProvider
{
public:
    static constexpr const char *TypeStart    = "start";
    static constexpr const char *TypeStop     = "stop";
    static constexpr const char *TypeLoop = "control/loop";
    static constexpr const char *TypeIfElse = "control/ifelse";
    static constexpr const char *TypeAdd = "math/add";
    static constexpr const char *TypeSubtract = "math/subtract";
    static constexpr const char *TypeMultiply = "math/multiply";
    static constexpr const char *TypeDivide = "math/divide";
    static constexpr const char *TypeErrorHandler = "system/error_handler";

    QString      providerId() const override;
    QStringList  componentTypeIds() const override;

    // Descriptor keys: id, title, category, defaultWidth, defaultHeight,
    //                  defaultColor, allowIncoming, allowOutgoing.
    QVariantMap  componentTypeDescriptor(const QString &componentTypeId) const override;

    QVariantMap  defaultComponentProperties(const QString &componentTypeId) const override;
};

#endif // CUSTOMIZECOMPONENTTYPEPROVIDER_H
