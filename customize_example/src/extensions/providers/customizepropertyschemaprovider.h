#ifndef CUSTOMIZEPROPERTYSCHEMAPROVIDER_H
#define CUSTOMIZEPROPERTYSCHEMAPROVIDER_H

#include <memory>
#include <vector>

#include <extensions/contracts/IPropertySchemaProvider.h>

// Customize implementation of IPropertySchemaProvider for the workflow domain.
// Schema definitions are authored via protobuf template messages and converted
// to QVariant rows through the runtime template adapter.
// Targets:
//   component/start, component/process, component/stop
//   connection/flow
// Each schema entry QVariantMap includes declarative layout/behavior keys such as:
// key, type, title, required, defaultValue, editor, section, order,
// hint, options, validation, visibleWhen.
class CustomizePropertySchemaProvider : public IPropertySchemaProvider
{
public:
    CustomizePropertySchemaProvider();

    QString providerId() const override;
    QStringList schemaTargets() const override;
    QVariantList propertySchema(const QString &targetId) const override;

private:
    std::vector<std::unique_ptr<IPropertySchemaProvider>> m_subProviders;
};

#endif // CUSTOMIZEPROPERTYSCHEMAPROVIDER_H
