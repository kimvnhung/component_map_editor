#include "SchemaSectionModel.h"

namespace cme::runtime {

SchemaFieldListModel::SchemaFieldListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int SchemaFieldListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_fields.size();
}

QVariant SchemaFieldListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_fields.size())
        return QVariant();

    const SchemaFieldDefinition &field = m_fields.at(index.row());
    switch (role) {
    case KeyRole:
        return field.key;
    case TitleRole:
        return field.title;
    case TypeRole:
        return field.typeName.isEmpty() ? schemaFieldTypeToString(field.type) : field.typeName;
    case TypeEnumRole:
        return static_cast<int>(field.type);
    case WidgetRole:
        return field.widgetName;
    case WidgetEnumRole:
        return static_cast<int>(field.widget);
    case RequiredRole:
        return field.required;
    case DefaultValueRole:
        return field.defaultValue;
    case SectionRole:
        return field.section;
    case OrderRole:
        return field.order;
    case HintRole:
        return field.hint;
    case PlaceholderRole:
        return field.placeholder;
    case OptionsSourceRole:
        return schemaOptionsSourceToString(field.optionsSource, field.optionsSourceKey);
    case OptionsSourceEnumRole:
        return static_cast<int>(field.optionsSource);
    case OptionsRole:
        return field.options;
    case VisibleWhenRole:
        return field.visibleWhen;
    case ValidationRole:
        return field.validation;
    case ValidRole:
        return field.valid;
    case SchemaErrorRole:
        return field.schemaError;
    default:
        break;
    }

    return QVariant();
}

QHash<int, QByteArray> SchemaFieldListModel::roleNames() const
{
    return {
        { KeyRole, "key" },
        { TitleRole, "title" },
        { TypeRole, "type" },
        { TypeEnumRole, "typeEnum" },
        { WidgetRole, "widget" },
        { WidgetEnumRole, "widgetEnum" },
        { RequiredRole, "required" },
        { DefaultValueRole, "defaultValue" },
        { SectionRole, "section" },
        { OrderRole, "order" },
        { HintRole, "hint" },
        { PlaceholderRole, "placeholder" },
        { OptionsSourceRole, "optionsSource" },
        { OptionsSourceEnumRole, "optionsSourceEnum" },
        { OptionsRole, "options" },
        { VisibleWhenRole, "visibleWhen" },
        { ValidationRole, "validation" },
        { ValidRole, "valid" },
        { SchemaErrorRole, "schemaError" }
    };
}

void SchemaFieldListModel::setFields(const QVector<SchemaFieldDefinition> &fields)
{
    beginResetModel();
    m_fields = fields;
    endResetModel();
}

int SchemaFieldListModel::size() const
{
    return m_fields.size();
}

QVariantMap SchemaFieldListModel::rowAt(int index) const
{
    if (index < 0 || index >= m_fields.size())
        return {};
    return SchemaFieldLegacyAdapter::toNormalizedRow(m_fields.at(index));
}

SchemaSectionListModel::SchemaSectionListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

SchemaSectionListModel::~SchemaSectionListModel()
{
    for (const SectionEntry &entry : m_sections)
        delete entry.fieldsModel;
}

int SchemaSectionListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_sections.size();
}

QVariant SchemaSectionListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_sections.size())
        return QVariant();

    const SectionEntry &section = m_sections.at(index.row());
    switch (role) {
    case IdRole:
        return section.id;
    case TitleRole:
        return section.title;
    case FieldsModelRole:
        return QVariant::fromValue(static_cast<QObject *>(section.fieldsModel));
    default:
        break;
    }

    return QVariant();
}

QHash<int, QByteArray> SchemaSectionListModel::roleNames() const
{
    return {
        { IdRole, "id" },
        { TitleRole, "title" },
        { FieldsModelRole, "fieldsModel" }
    };
}

void SchemaSectionListModel::setSections(const QVector<SchemaSectionDefinition> &sections)
{
    beginResetModel();
    for (const SectionEntry &entry : m_sections)
        delete entry.fieldsModel;
    m_sections.clear();
    m_sections.reserve(sections.size());

    for (const SchemaSectionDefinition &section : sections) {
        SectionEntry entry;
        entry.id = section.id;
        entry.title = section.title;
        entry.fieldsModel = new SchemaFieldListModel();
        entry.fieldsModel->setFields(section.fields);
        m_sections.append(entry);
    }

    endResetModel();
}

int SchemaSectionListModel::size() const
{
    return m_sections.size();
}

QVariantMap SchemaSectionListModel::rowAt(int index) const
{
    if (index < 0 || index >= m_sections.size())
        return {};

    const SectionEntry &entry = m_sections.at(index);
    return {
        { QStringLiteral("id"), entry.id },
        { QStringLiteral("title"), entry.title },
        { QStringLiteral("fieldsModel"), QVariant::fromValue(static_cast<QObject *>(entry.fieldsModel)) }
    };
}

} // namespace cme::runtime
