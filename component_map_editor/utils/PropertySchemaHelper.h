#ifndef PROPERTYSCHEMAHELPER_H
#define PROPERTYSCHEMAHELPER_H

#include "extensions/runtime/SchemaFieldDefinition.h"
#include "provider_templates.pb.h"
#include "extensions/runtime/SchemaOptionsSpec.h"

namespace cme::helper::property_schemas
{
using cme::runtime::SchemaFieldSection;
using cme::runtime::SchemaFieldType;
using cme::runtime::SchemaFieldWidget;

class Field
{
public:
    Field() = default;

    Field(const QString& key);
    Field& type(cme::runtime::SchemaFieldType type);
    Field& title(const QString& title);
    Field& required();

    // Define Widget
    Field& textArea();
    Field& textField();
    Field& dropdown(SchemaOptionsSpec spec = SchemaOptionsSpec::none());
    Field& checkbox();
    Field& spinBox(double min = 0.0, double max = 100.0);

    Field& defaultValue(const QVariant& value);
    Field& section(SchemaFieldSection section);
    Field& order(int order);
    Field& hint(const QString& hint);
    Field& validation(const QVariantMap& validation);
    Field& visibleWhen(const QVariantMap& visibleWhen);
    Field& options(const QVariantList& options);
    Field& optionsSource(cme::runtime::SchemaOptionsSource source);
    Field& customOptionsSource(const QString& sourceId);
    Field& extra(const QVariantMap& extra);

    cme::templates::v1::PropertySchemaFieldTemplate buildTemplate() const;

private:
    QString m_key;
    cme::runtime::SchemaFieldType m_type = cme::runtime::SchemaFieldType::String;
    QString m_title;
    bool m_required = false;
    QVariant m_defaultValue;
    cme::runtime::SchemaFieldWidget m_widget = cme::runtime::SchemaFieldWidget::TextField;
    cme::runtime::SchemaFieldSection m_section = cme::runtime::SchemaFieldSection::Identity;
    int m_order = 0;
    QString m_hint;
    QVariantMap m_validation;
    QVariantMap m_visibleWhen;
    QVariantList m_options;
    cme::runtime::SchemaOptionsSource m_optionsSource = cme::runtime::SchemaOptionsSource::None;
    QString m_customOptionsSource;
    QVariantMap m_extra;
};

cme::templates::v1::PropertySchemaFieldTemplate
makeField(const char* key, const char* type, const char* title, bool required, const QVariant& defaultValue,
          const char* editor, const char* section, int order, const QString& hint = QString(),
          const QVariantMap& validation = {}, const QVariantMap& visibleWhen = {}, const QVariantList& options = {},
          const QVariantMap& extra = {});

cme::templates::v1::PropertySchemaFieldTemplate
makeField(const char* key, cme::runtime::SchemaFieldType type, const char* title, bool required,
          const QVariant& defaultValue, cme::runtime::SchemaFieldWidget widget,
          cme::runtime::SchemaFieldSection section, int order, const QString& hint = QString(),
          const QVariantMap& validation = {}, const QVariantMap& visibleWhen = {}, const QVariantList& options = {},
          cme::runtime::SchemaOptionsSource optionsSource = cme::runtime::SchemaOptionsSource::None,
          const QString& customOptionsSource = QString(), const QVariantMap& extra = {});

void addTarget(cme::templates::v1::PropertySchemaTemplateBundle* bundle, const char* targetId,
               const std::initializer_list<cme::templates::v1::PropertySchemaFieldTemplate>& fields);

} // namespace cme::helper::property_schemas

#endif // PROPERTYSCHEMAHELPER_H
