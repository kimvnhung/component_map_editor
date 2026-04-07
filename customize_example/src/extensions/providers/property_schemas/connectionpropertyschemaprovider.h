#ifndef CUSTOMIZE_PROPERTYSCHEMAS_CONNECTIONPROPERTYSCHEMAPROVIDER_H
#define CUSTOMIZE_PROPERTYSCHEMAS_CONNECTIONPROPERTYSCHEMAPROVIDER_H

#include <extensions/contracts/IPropertySchemaProvider.h>

class ConnectionPropertySchemaProvider : public IPropertySchemaProvider
{
public:
    QString providerId() const override;
    QStringList schemaTargets() const override;
    QVariantList propertySchema(const QString &targetId) const override;
};

#endif // CUSTOMIZE_PROPERTYSCHEMAS_CONNECTIONPROPERTYSCHEMAPROVIDER_H
