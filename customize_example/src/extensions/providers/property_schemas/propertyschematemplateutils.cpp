#include "propertyschematemplateutils.h"

#include "extensions/runtime/templates/TemplateProtoHelpers.h"

namespace customize::property_schemas {

cme::templates::v1::PropertySchemaFieldTemplate makeField(
    const char *key,
    const char *type,
    const char *title,
    bool required,
    const QVariant &defaultValue,
    const char *editor,
    const char *section,
    int order,
    const QString &hint,
    const QVariantMap &validation,
    const QVariantMap &visibleWhen,
    const QVariantList &options,
    const QVariantMap &extra)
{
    cme::templates::v1::PropertySchemaFieldTemplate field;
    field.set_key(key);
    field.set_type(type);
    field.set_title(title);
    field.set_required(required);
    field.set_editor(editor);
    field.set_section(section);
    field.set_order(order);

    if (!hint.isEmpty())
        field.set_hint(hint.toStdString());

    *field.mutable_default_value() = cme::runtime::templates::variantToProtoValue(defaultValue);

    for (const QVariant &option : options)
        *field.add_options() = cme::runtime::templates::variantToProtoValue(option);

    for (auto it = validation.constBegin(); it != validation.constEnd(); ++it)
        (*field.mutable_validation())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());

    for (auto it = visibleWhen.constBegin(); it != visibleWhen.constEnd(); ++it)
        (*field.mutable_visible_when())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());

    for (auto it = extra.constBegin(); it != extra.constEnd(); ++it)
        (*field.mutable_extra())[it.key().toStdString()] = cme::runtime::templates::variantToProtoValue(it.value());

    return field;
}

cme::templates::v1::PropertySchemaFieldTemplate makeField(
    const char *key,
    cme::runtime::SchemaFieldType type,
    const char *title,
    bool required,
    const QVariant &defaultValue,
    cme::runtime::SchemaFieldWidget widget,
    const char *section,
    int order,
    const QString &hint,
    const QVariantMap &validation,
    const QVariantMap &visibleWhen,
    const QVariantList &options,
    cme::runtime::SchemaOptionsSource optionsSource,
    const QString &customOptionsSource,
    const QVariantMap &extra)
{
    const QByteArray typeName = cme::runtime::schemaFieldTypeToString(type).toUtf8();
    const QByteArray widgetName = cme::runtime::schemaFieldWidgetToString(widget).toUtf8();

    QVariantMap mergedExtra = extra;
    const QString optionsSourceValue = cme::runtime::schemaOptionsSourceToString(optionsSource,
                                                                                  customOptionsSource);
    if (!optionsSourceValue.isEmpty() && !mergedExtra.contains(QStringLiteral("optionsSource")))
        mergedExtra.insert(QStringLiteral("optionsSource"), optionsSourceValue);

    return makeField(key,
                     typeName.constData(),
                     title,
                     required,
                     defaultValue,
                     widgetName.constData(),
                     section,
                     order,
                     hint,
                     validation,
                     visibleWhen,
                     options,
                     mergedExtra);
}

cme::templates::v1::PropertySchemaFieldTemplate makeTokenKeyField(
    const char *key,
    const char *title,
    bool required,
    const QVariant &defaultValue,
    const char *section,
    int order,
    const QString &hint)
{
    QString resolvedHint = hint;
    if (!resolvedHint.isEmpty())
        resolvedHint.append(QStringLiteral(" "));
    resolvedHint.append(QStringLiteral("Token options are sourced from connection token keys, execution-state keys, and schema key defaults."));

    return makeField(key,
                     cme::runtime::SchemaFieldType::String,
                     title,
                     required,
                     defaultValue,
                     cme::runtime::SchemaFieldWidget::Dropdown,
                     section,
                     order,
                     resolvedHint,
                     {},
                     {},
                     {},
                     cme::runtime::SchemaOptionsSource::TokenKeys);
}

void addTarget(
    cme::templates::v1::PropertySchemaTemplateBundle *bundle,
    const char *targetId,
    const std::initializer_list<cme::templates::v1::PropertySchemaFieldTemplate> &fields)
{
    cme::templates::v1::PropertySchemaTargetTemplate *target = bundle->add_targets();
    target->set_target_id(targetId);
    for (const auto &field : fields)
        *target->add_entries() = field;
}

} // namespace customize::property_schemas
