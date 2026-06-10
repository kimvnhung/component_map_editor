#ifndef FILECOMPONENTTYPEPROVIDER_H
#define FILECOMPONENTTYPEPROVIDER_H

#include <extensions/contracts/IComponentTypeProvider.h>

class FileComponentTypeProvider : public IComponentTypeProvider
{
public:
    QString providerId() const override;
    QStringList componentTypeIds() const override;
    QVariantMap componentTypeDescriptor(const QString &componentTypeId) const override;
    QVariantMap defaultComponentProperties(const QString &componentTypeId) const override;
};

#endif // FILECOMPONENTTYPEPROVIDER_H
