#ifndef SAMPLEPROPERTYSCHEMAPROVIDER_H
#define SAMPLEPROPERTYSCHEMAPROVIDER_H

#include "extensions/contracts/IPropertySchemaProvider.h"

// Sample implementation of IPropertySchemaProvider for the workflow domain.
// Targets:
//   component/start, component/task, component/decision, component/end
//   connection/flow
// Each schema entry QVariantMap has keys: key, type, title, required, defaultValue, editor.
class SamplePropertySchemaProvider : public IPropertySchemaProvider
{
public:
    QString      providerId() const override;
    QStringList  schemaTargets() const override;
    QVariantList propertySchema(const QString &targetId) const override;
};

#endif // SAMPLEPROPERTYSCHEMAPROVIDER_H
