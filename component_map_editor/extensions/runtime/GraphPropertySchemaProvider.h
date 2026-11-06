#ifndef GRAPHPROPERTYSCHEMAPROVIDER_H
#define GRAPHPROPERTYSCHEMAPROVIDER_H

#include "extensions/contracts/IPropertySchemaProvider.h"

/**
 * @brief The GraphPropertySchemaProvider class
 * Provides property schema definitions for graph-level properties in the workflow editor.
 */

class GraphPropertySchemaProvider : public IPropertySchemaProvider
{
public:
    QString providerId() const override;
    QStringList schemaTargets() const override;
    QVariantList propertySchema(const QString &targetId) const override;
};

#endif // GRAPHPROPERTYSCHEMAPROVIDER_H
