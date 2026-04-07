#ifndef SCHEMAFIELDDEFINITION_H
#define SCHEMAFIELDDEFINITION_H

#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace cme::runtime {

enum class SchemaFieldType {
    Unknown = 0,
    String,
    Number,
    Boolean,
    Enum
};

enum class SchemaFieldWidget {
    Unknown = 0,
    TextField,
    TextArea,
    Dropdown,
    Checkbox,
    SpinBox,
    SchemaError
};

enum class SchemaOptionsSource {
    None = 0,
    TokenKeys,
    TokenKeyOptions,
    Custom
};

QString schemaFieldTypeToString(SchemaFieldType value);
SchemaFieldType schemaFieldTypeFromString(const QString &value);

QString schemaFieldWidgetToString(SchemaFieldWidget value);
SchemaFieldWidget schemaFieldWidgetFromString(const QString &value);

QString schemaOptionsSourceToString(SchemaOptionsSource value, const QString &customValue = QString());
SchemaOptionsSource schemaOptionsSourceFromString(const QString &value);

struct SchemaFieldDefinition {
    QString key;
    QString title;
    SchemaFieldType type = SchemaFieldType::Unknown;
    QString typeName;
    SchemaFieldWidget widget = SchemaFieldWidget::Unknown;
    QString widgetName;
    bool required = false;
    QVariant defaultValue;
    QString section;
    int order = 0;
    QString hint;
    QString placeholder;
    SchemaOptionsSource optionsSource = SchemaOptionsSource::None;
    QString optionsSourceKey;
    QVariantList options;
    QVariantMap visibleWhen;
    QVariantMap validation;
    bool valid = true;
    QString schemaError;
};

class SchemaFieldLegacyAdapter
{
public:
    static SchemaFieldDefinition fromLegacyRow(const QVariantMap &raw, const QString &targetId);
    static QVariantMap toNormalizedRow(const SchemaFieldDefinition &field);
};

} // namespace cme::runtime

#endif // SCHEMAFIELDDEFINITION_H
