#ifndef CUSTOMIZE_PROPERTYSCHEMAS_CONTROLPROPERTYSCHEMAPROVIDER_H
#define CUSTOMIZE_PROPERTYSCHEMAS_CONTROLPROPERTYSCHEMAPROVIDER_H

#include <extensions/contracts/IPropertySchemaProvider.h>

class ControlPropertySchemaProvider : public IPropertySchemaProvider
{
public:
    QString providerId() const override;
    QStringList schemaTargets() const override;
    QVariantList propertySchema(const QString &targetId) const override;
};

#endif // CUSTOMIZE_PROPERTYSCHEMAS_CONTROLPROPERTYSCHEMAPROVIDER_H
