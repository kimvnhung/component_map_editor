#ifndef FACTORYCOMPONENTTYPEPROVIDER_H
#define FACTORYCOMPONENTTYPEPROVIDER_H

#include <extensions/contracts/IComponentTypeProvider.h>

class FactoryComponentTypeProvider : public IComponentTypeProvider
{
public:
    static constexpr const char *TypeFruitProducer = "factory/fruit_producer";
    static constexpr const char *TypeStore = "factory/store";
    static constexpr const char *TypeEmployee = "factory/employee";
    static constexpr const char *TypeSeller = "factory/seller";
    static constexpr const char *TypeManager = "factory/manager";

    QString providerId() const override;
    QStringList componentTypeIds() const override;

    QVariantMap componentTypeDescriptor(const QString &componentTypeId) const override;
    QVariantMap defaultComponentProperties(const QString &componentTypeId) const override;
};

#endif // FACTORYCOMPONENTTYPEPROVIDER_H
