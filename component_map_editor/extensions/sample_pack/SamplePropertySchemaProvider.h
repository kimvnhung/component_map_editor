#ifndef SAMPLEPROPERTYSCHEMAPROVIDER_H
#define SAMPLEPROPERTYSCHEMAPROVIDER_H

#include "extensions/contracts/IPropertySchemaProvider.h"

// Sample implementation of IPropertySchemaProvider for the workflow domain.
// Targets:
//   component/start, component/task, component/decision, component/end
//   connection/flow
// Each schema entry QVariantMap includes declarative layout/behavior keys such as:
// key, type, title, required, defaultValue, editor, section, order,
// hint, options, validation, visibleWhen.
class SamplePropertySchemaProvider : public IPropertySchemaProvider
{
public:
    QString      providerId() const override;
    QStringList  schemaTargets() const override;
    QVariantList propertySchema(const QString &targetId) const override;
};

#endif // SAMPLEPROPERTYSCHEMAPROVIDER_H
