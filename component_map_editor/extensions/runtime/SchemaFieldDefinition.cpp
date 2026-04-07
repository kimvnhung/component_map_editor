#include "SchemaFieldDefinition.h"

namespace cme::runtime {

namespace {

QString normalized(const QString &value)
{
    return value.trimmed();
}

} // namespace

QString schemaFieldTypeToString(SchemaFieldType value)
{
    switch (value) {
    case SchemaFieldType::String:
        return QStringLiteral("string");
    case SchemaFieldType::Number:
        return QStringLiteral("number");
    case SchemaFieldType::Boolean:
        return QStringLiteral("bool");
    case SchemaFieldType::Enum:
        return QStringLiteral("enum");
    case SchemaFieldType::Unknown:
        break;
    }
    return QString();
}

SchemaFieldType schemaFieldTypeFromString(const QString &value)
{
    const QString key = normalized(value).toLower();
    if (key == QStringLiteral("string"))
        return SchemaFieldType::String;
    if (key == QStringLiteral("number") || key == QStringLiteral("int") || key == QStringLiteral("integer")
        || key == QStringLiteral("double") || key == QStringLiteral("float"))
        return SchemaFieldType::Number;
    if (key == QStringLiteral("bool") || key == QStringLiteral("boolean"))
        return SchemaFieldType::Boolean;
    if (key == QStringLiteral("enum"))
        return SchemaFieldType::Enum;
    return SchemaFieldType::Unknown;
}

QString schemaFieldWidgetToString(SchemaFieldWidget value)
{
    switch (value) {
    case SchemaFieldWidget::TextField:
        return QStringLiteral("textfield");
    case SchemaFieldWidget::TextArea:
        return QStringLiteral("textarea");
    case SchemaFieldWidget::Dropdown:
        return QStringLiteral("dropdown");
    case SchemaFieldWidget::Checkbox:
        return QStringLiteral("checkbox");
    case SchemaFieldWidget::SpinBox:
        return QStringLiteral("spinbox");
    case SchemaFieldWidget::SchemaError:
        return QStringLiteral("schema_error");
    case SchemaFieldWidget::Unknown:
        break;
    }
    return QString();
}

SchemaFieldWidget schemaFieldWidgetFromString(const QString &value)
{
    const QString key = normalized(value).toLower();
    if (key == QStringLiteral("textfield"))
        return SchemaFieldWidget::TextField;
    if (key == QStringLiteral("textarea"))
        return SchemaFieldWidget::TextArea;
    if (key == QStringLiteral("dropdown"))
        return SchemaFieldWidget::Dropdown;
    if (key == QStringLiteral("checkbox"))
        return SchemaFieldWidget::Checkbox;
    if (key == QStringLiteral("spinbox"))
        return SchemaFieldWidget::SpinBox;
    if (key == QStringLiteral("schema_error"))
        return SchemaFieldWidget::SchemaError;
    return SchemaFieldWidget::Unknown;
}

QString schemaOptionsSourceToString(SchemaOptionsSource value, const QString &customValue)
{
    switch (value) {
    case SchemaOptionsSource::None:
        return QString();
    case SchemaOptionsSource::TokenKeys:
        return QStringLiteral("tokenKeys");
    case SchemaOptionsSource::TokenKeyOptions:
        return QStringLiteral("tokenKeyOptions");
    case SchemaOptionsSource::Custom:
        return customValue.trimmed();
    }
    return QString();
}

SchemaOptionsSource schemaOptionsSourceFromString(const QString &value)
{
    const QString key = normalized(value);
    if (key.isEmpty())
        return SchemaOptionsSource::None;
    if (key == QStringLiteral("tokenKeys"))
        return SchemaOptionsSource::TokenKeys;
    if (key == QStringLiteral("tokenKeyOptions"))
        return SchemaOptionsSource::TokenKeyOptions;
    return SchemaOptionsSource::Custom;
}

SchemaFieldDefinition SchemaFieldLegacyAdapter::fromLegacyRow(const QVariantMap &raw, const QString &targetId)
{
    SchemaFieldDefinition field;

    field.key = raw.value(QStringLiteral("key")).toString().trimmed();
    const QString title = raw.value(QStringLiteral("title")).toString().trimmed();
    field.title = title.isEmpty() ? field.key : title;

    const QString typeName = raw.value(QStringLiteral("type")).toString().trimmed();
    field.typeName = typeName;
    field.type = schemaFieldTypeFromString(typeName);

    const QString editorLegacy = raw.value(QStringLiteral("editor")).toString().trimmed();
    const QString widget = raw.value(QStringLiteral("widget")).toString().trimmed();
    const QString resolvedWidget = widget.isEmpty() ? editorLegacy : widget;
    field.widgetName = resolvedWidget;
    field.widget = schemaFieldWidgetFromString(resolvedWidget);

    field.required = raw.value(QStringLiteral("required")).toBool();
    field.defaultValue = raw.value(QStringLiteral("defaultValue"));
    field.section = raw.value(QStringLiteral("section")).toString();
    field.order = raw.value(QStringLiteral("order")).toInt();
    field.hint = raw.value(QStringLiteral("hint")).toString();
    field.placeholder = raw.value(QStringLiteral("placeholder")).toString();
    field.options = raw.value(QStringLiteral("options")).toList();
    field.visibleWhen = raw.value(QStringLiteral("visibleWhen")).toMap();
    field.validation = raw.value(QStringLiteral("validation")).toMap();

    field.optionsSourceKey = raw.value(QStringLiteral("optionsSource")).toString();
    field.optionsSource = schemaOptionsSourceFromString(field.optionsSourceKey);

    if (field.key.isEmpty()) {
        field.valid = false;
        field.schemaError = QStringLiteral("Schema field in target '%1' is missing key.").arg(targetId);
        field.widget = SchemaFieldWidget::SchemaError;
        field.widgetName = QStringLiteral("schema_error");
        return field;
    }

    if (field.widgetName.isEmpty()) {
        field.valid = false;
        field.schemaError = QStringLiteral("Schema field '%1' in target '%2' is missing widget/editor.")
                                .arg(field.key, targetId);
        field.widget = SchemaFieldWidget::SchemaError;
        field.widgetName = QStringLiteral("schema_error");
        return field;
    }

    field.valid = true;
    field.schemaError.clear();
    return field;
}

QVariantMap SchemaFieldLegacyAdapter::toNormalizedRow(const SchemaFieldDefinition &field)
{
    const QString typeName = field.typeName.isEmpty() ? schemaFieldTypeToString(field.type) : field.typeName;
    const QString widgetName = field.valid ? field.widgetName : QStringLiteral("schema_error");
    const QString optionsSource = schemaOptionsSourceToString(field.optionsSource, field.optionsSourceKey);

    return {
        { QStringLiteral("key"), field.key },
        { QStringLiteral("title"), field.title },
        { QStringLiteral("type"), typeName },
        { QStringLiteral("typeEnum"), static_cast<int>(field.type) },
        { QStringLiteral("widget"), widgetName },
        { QStringLiteral("widgetEnum"), static_cast<int>(field.widget) },
        { QStringLiteral("required"), field.required },
        { QStringLiteral("defaultValue"), field.defaultValue },
        { QStringLiteral("section"), field.section },
        { QStringLiteral("order"), field.order },
        { QStringLiteral("hint"), field.hint },
        { QStringLiteral("placeholder"), field.placeholder },
        { QStringLiteral("optionsSource"), optionsSource },
        { QStringLiteral("optionsSourceEnum"), static_cast<int>(field.optionsSource) },
        { QStringLiteral("options"), field.options },
        { QStringLiteral("visibleWhen"), field.visibleWhen },
        { QStringLiteral("validation"), field.validation },
        { QStringLiteral("valid"), field.valid },
        { QStringLiteral("schemaError"), field.schemaError }
    };
}

} // namespace cme::runtime
