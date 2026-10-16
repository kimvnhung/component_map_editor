#include "PropertySchemaHelper.h"

#include "extensions/runtime/templates/TemplateProtoHelpers.h"

namespace cme::helper::property_schemas
{
Field::Field(const QString& key)
    : m_key(key), m_widget(cme::runtime::SchemaFieldWidget::SchemaError), m_required(false), m_defaultValue(QVariant()),
      m_section(cme::runtime::SchemaFieldSection::Identity), m_order(0)
{
}

Field& Field::type(cme::runtime::SchemaFieldType type)
{
    m_type = type;
    return *this;
}

Field& Field::title(const QString& title)
{
    m_title = title;
    return *this;
}

Field& Field::required()
{
    m_required = true;
    return *this;
}

Field& Field::defaultValue(const QVariant& value)
{
    m_defaultValue = value;
    return *this;
}

Field& Field::section(SchemaFieldSection section)
{
    m_section = section;
    return *this;
}

Field& Field::order(int order)
{
    m_order = order;
    return *this;
}

Field& Field::hint(const QString& hint)
{
    m_hint = hint;
    return *this;
}
Field& Field::validation(const QVariantMap& validation)
{
    m_validation = validation;
    return *this;
}

Field& Field::visibleWhen(const QVariantMap& visibleWhen)
{
    m_visibleWhen = visibleWhen;
    return *this;
}

Field& Field::options(const QVariantList& options)
{
    m_options = options;
    return *this;
}

Field& Field::optionsSource(cme::runtime::SchemaOptionsSource source)
{
    m_optionsSource = source;
    return *this;
}

Field& Field::customOptionsSource(const QString& sourceId)
{
    m_customOptionsSource = sourceId;
    return *this;
}

Field& Field::extra(const QVariantMap& extra)
{
    m_extra = extra;
    return *this;
}

Field& Field::textArea()
{
    m_widget = cme::runtime::SchemaFieldWidget::TextArea;
    return *this;
}

Field& Field::textField()
{
    m_widget = cme::runtime::SchemaFieldWidget::TextField;
    return *this;
}

Field& Field::dropdown(cme::runtime::SchemaOptionsSpec spec)
{
    m_widget = cme::runtime::SchemaFieldWidget::Dropdown;
    if (spec.sourceId == "fixed")
    {
        m_options = spec.items;
    }
    else
    {
        // m_optionsSource = spec.sourceId == OptionsSources::TokenKeys
        //                       ? cme::runtime::SchemaOptionsSource::TokenKeys
        //                       : cme::runtime::SchemaOptionsSource::TokenKeyOptions;
    }
    return *this;
}

Field& Field::checkbox()
{
    m_widget = cme::runtime::SchemaFieldWidget::Checkbox;
    return *this;
}

Field& Field::spinBox(double min, double max)
{
    m_widget = cme::runtime::SchemaFieldWidget::SpinBox;
    if (min == -1 || max == -1)
    {
        m_validation.clear();
    }
    else
    {
        m_validation.insert("min", min);
        m_validation.insert("max", max);
    }
    return *this;
}

cme::templates::v1::PropertySchemaFieldTemplate Field::build() const
{
    const QByteArray typeName = cme::runtime::schemaFieldTypeToString(m_type).toUtf8();
    const QByteArray widgetName = cme::runtime::schemaFieldWidgetToString(m_widget).toUtf8();
    const QByteArray sectionName = cme::runtime::schemaFieldSectionToString(m_section).toUtf8();

    QVariantMap mergedExtra = m_extra;
    const QString optionsSourceValue =
        cme::runtime::schemaOptionsSourceToString(m_optionsSource, m_customOptionsSource);

    if (!optionsSourceValue.isEmpty() && !mergedExtra.contains(QStringLiteral("optionsSource")))
    {
        mergedExtra.insert(QStringLiteral("optionsSource"), optionsSourceValue);
    }

    cme::templates::v1::PropertySchemaFieldTemplate field;
    field.set_key(m_key.toStdString());
    field.set_type(typeName);
    field.set_title(m_title.toStdString());
    field.set_required(m_required);
    field.set_editor(widgetName);
    field.set_section(sectionName);
    field.set_order(m_order);

    if (!m_hint.isEmpty())
    {
        field.set_hint(m_hint.toStdString());
    }

    *field.mutable_default_value() = cme::runtime::templates::variantToProtoValue(m_defaultValue);

    for (const QVariant& option : m_options)
    {
        *field.add_options() = cme::runtime::templates::variantToProtoValue(option);
    }

    for (auto it = m_validation.constBegin(); it != m_validation.constEnd(); ++it)
    {
        (*field.mutable_validation())[it.key().toStdString()] =
            cme::runtime::templates::variantToProtoValue(it.value());
    }

    for (auto it = m_visibleWhen.constBegin(); it != m_visibleWhen.constEnd(); ++it)
    {
        (*field.mutable_visible_when())[it.key().toStdString()] =
            cme::runtime::templates::variantToProtoValue(it.value());
    }

    for (auto it = mergedExtra.constBegin(); it != mergedExtra.constEnd(); ++it)
    {
        (*field.mutable_extra())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());
    }

    return field;
}

// cme::templates::v1::PropertySchemaFieldTemplate
// makeField(const char* key, const char* type, const char* title, bool required, const QVariant& defaultValue,
//           const char* editor, const char* section, int order, const QString& hint, const QVariantMap& validation,
//           const QVariantMap& visibleWhen, const QVariantList& options, const QVariantMap& extra)
// {
// }

// cme::templates::v1::PropertySchemaFieldTemplate
// makeField(const char* key, cme::runtime::SchemaFieldType type, const char* title, bool required,
//           const QVariant& defaultValue, cme::runtime::SchemaFieldWidget widget,
//           cme::runtime::SchemaFieldSection section, int order, const QString& hint, const QVariantMap& validation,
//           const QVariantMap& visibleWhen, const QVariantList& options, cme::runtime::SchemaOptionsSource
//           optionsSource, const QString& customOptionsSource, const QVariantMap& extra)
// {
//     const QByteArray typeName = cme::runtime::schemaFieldTypeToString(type).toUtf8();
//     const QByteArray widgetName = cme::runtime::schemaFieldWidgetToString(widget).toUtf8();
//     const QByteArray sectionName = cme::runtime::schemaFieldSectionToString(section).toUtf8();

//     QVariantMap mergedExtra = extra;
//     const QString optionsSourceValue = cme::runtime::schemaOptionsSourceToString(optionsSource, customOptionsSource);

//     if (!optionsSourceValue.isEmpty() && !mergedExtra.contains(QStringLiteral("optionsSource")))
//     {
//         mergedExtra.insert(QStringLiteral("optionsSource"), optionsSourceValue);
//     }

//     return makeField(key, typeName.constData(), title, required, defaultValue, widgetName.constData(),
//                      sectionName.constData(), order, hint, validation, visibleWhen, options, mergedExtra);
// }

void addTarget(cme::templates::v1::PropertySchemaTemplateBundle* bundle, const char* targetId,
               const std::initializer_list<cme::templates::v1::PropertySchemaFieldTemplate>& fields)
{
    cme::templates::v1::PropertySchemaTargetTemplate* target = bundle->add_targets();
    target->set_target_id(targetId);

    for (const auto& field : fields)
    {
        *target->add_entries() = field;
    }
}

} // namespace cme::helper::property_schemas