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
    static constexpr const char *TypeCondition = "condition";
    static constexpr const char *TypeProcess  = "process";
    static constexpr const char *TypeStop     = "stop";
    static constexpr const char *TypeAdd = "math/add";
    static constexpr const char *TypeSubtract = "math/subtract";
    static constexpr const char *TypeMultiply = "math/multiply";
    static constexpr const char *TypeDivide = "math/divide";
    static constexpr const char *TypeSqrtNewton = "workflow/sqrt_newton";
    static constexpr const char *TypeQuadratic = "workflow/quadratic";
    static constexpr const char *TypeErrorHandler = "system/error_handler";

    QString      providerId() const override;
    QStringList  componentTypeIds() const override;

    // Descriptor keys: id, title, category, defaultWidth, defaultHeight,
    //                  defaultColor, allowIncoming, allowOutgoing.
    QVariantMap  componentTypeDescriptor(const QString &componentTypeId) const override;

    // Default property keys for each type:
    //   start   -> inputNumber(0)
    //   process -> addValue(9), description("")
    //   stop    -> (empty defaults)
    QVariantMap  defaultComponentProperties(const QString &componentTypeId) const override;
};

#endif // CUSTOMIZECOMPONENTTYPEPROVIDER_H
