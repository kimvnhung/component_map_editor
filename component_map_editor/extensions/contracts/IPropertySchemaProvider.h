#ifndef IPROPERTYSCHEMAPROVIDER_H
#define IPROPERTYSCHEMAPROVIDER_H

#include <QString>
#include <QStringList>
#include <QtPlugin>
#include <QVariantList>

class IPropertySchemaProvider
{
public:
    virtual ~IPropertySchemaProvider() = default;

    virtual QString providerId() const = 0;

    // Returns IDs like "component/<typeId>" or "connection/<typeId>".
    virtual QStringList schemaTargets() const = 0;

    // Returns schema entries. Recommended keys per entry:
    // key, type, title, required, defaultValue, editor.
    virtual QVariantList propertySchema(const QString &targetId) const = 0;
};

#define COMPONENT_MAP_EDITOR_IID_PROPERTY_SCHEMA_PROVIDER "ComponentMapEditor.Extensions.IPropertySchemaProvider/1.0"
Q_DECLARE_INTERFACE(IPropertySchemaProvider, COMPONENT_MAP_EDITOR_IID_PROPERTY_SCHEMA_PROVIDER)

#endif // IPROPERTYSCHEMAPROVIDER_H
