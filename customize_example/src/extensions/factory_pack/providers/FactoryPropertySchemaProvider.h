#ifndef FACTORYPROPERTYSCHEMAPROVIDER_H
#define FACTORYPROPERTYSCHEMAPROVIDER_H


#include <extensions/contracts/IPropertySchemaProvider.h>

class FactoryPropertySchemaProvider : public IPropertySchemaProvider
{
public:
    FactoryPropertySchemaProvider();

    QString providerId() const override;
    QStringList schemaTargets() const override;
    QVariantList propertySchema(const QString &targetId) const override;
};

#endif // FACTORYPROPERTYSCHEMAPROVIDER_H
