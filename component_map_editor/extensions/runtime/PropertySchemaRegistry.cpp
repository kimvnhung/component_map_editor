#include "PropertySchemaRegistry.h"

#include <algorithm>
#include <QtAlgorithms>

#include "PublicApiContractAdapter.h"
#include "SchemaFieldDefinition.h"

namespace {

QVariantMap makeFallbackField(const QString &title,
                              const QString &key,
                              const QString &widget,
                              const QVariant &defaultValue,
                              const QString &section,
                              int order)
{
    return {
        { QStringLiteral("key"), key },
        { QStringLiteral("title"), title },
        { QStringLiteral("type"), QStringLiteral("string") },
        { QStringLiteral("widget"), widget },
        { QStringLiteral("required"), false },
        { QStringLiteral("defaultValue"), defaultValue },
        { QStringLiteral("section"), section },
        { QStringLiteral("order"), order },
        { QStringLiteral("hint"), QString() },
        { QStringLiteral("visibleWhen"), QVariantMap() },
        { QStringLiteral("validation"), QVariantMap() },
        { QStringLiteral("valid"), true },
        { QStringLiteral("schemaError"), QString() }
    };
}

} // namespace

PropertySchemaRegistry::PropertySchemaRegistry(QObject *parent)
    : QObject(parent)
{}

void PropertySchemaRegistry::rebuildFromRegistry(const ExtensionContractRegistry &registry)
{
    m_rowsByTarget.clear();
    qDeleteAll(m_sectionModelsByTarget);
    m_sectionModelsByTarget.clear();

    const QList<const IPropertySchemaProvider *> providers = registry.propertySchemaProviders();
    for (const IPropertySchemaProvider *provider : providers) {
        if (!provider)
            continue;

        const QStringList targets = provider->schemaTargets();
        for (const QString &targetId : targets) {
            if (targetId.isEmpty())
                continue;

            const QVariantList normalized = normalizeRows(provider->propertySchema(targetId), targetId);
            if (normalized.isEmpty())
                continue;

            QVariantList merged = m_rowsByTarget.value(targetId);
            merged.append(normalized);
            m_rowsByTarget.insert(targetId, merged);
        }
    }

    emit schemasChanged();
}

bool PropertySchemaRegistry::hasTarget(const QString &targetId) const
{
    return m_rowsByTarget.contains(targetId);
}

QVariantList PropertySchemaRegistry::schemaForTarget(const QString &targetId) const
{
    return resolvedSchemaRows(targetId);
}

bool PropertySchemaRegistry::schemaForTargetTyped(
    const QString &targetId,
    cme::publicapi::v1::PropertySchemaResponse *out,
    QString *error) const
{
    if (!out) {
        if (error)
            *error = QStringLiteral("PropertySchemaResponse output pointer is null");
        return false;
    }

    out->Clear();
    out->mutable_status()->set_success(true);

    const QVariantList rows = resolvedSchemaRows(targetId);
    for (const QVariant &value : rows) {
        const QVariantMap row = value.toMap();
        if (row.isEmpty())
            continue;

        cme::publicapi::v1::PropertySchemaEntry *entry = out->add_entries();
        QString conversionError;
        if (!cme::runtime::PublicApiContractAdapter::toPropertySchemaEntry(row, entry, &conversionError)) {
            out->mutable_status()->set_success(false);
            out->mutable_status()->set_error_code("SCHEMA_CONVERSION_FAILED");
            out->mutable_status()->set_error_message(conversionError.toStdString());
            if (error)
                *error = conversionError;
            return false;
        }
    }

    return true;
}

QVariantList PropertySchemaRegistry::sectionedSchemaForTarget(const QString &targetId) const
{
    return sectionizeRows(resolvedSchemaRows(targetId));
}

QObject *PropertySchemaRegistry::typedSectionModelForTarget(const QString &targetId)
{
    if (targetId.isEmpty())
        return nullptr;

    if (m_sectionModelsByTarget.contains(targetId))
        return m_sectionModelsByTarget.value(targetId);

    auto *model = new cme::runtime::SchemaSectionListModel(this);
    model->setSections(sectionizeTypedRows(resolvedSchemaRows(targetId), targetId));
    m_sectionModelsByTarget.insert(targetId, model);
    return model;
}

QVariantList PropertySchemaRegistry::componentSchema(const QString &componentTypeId) const
{
    return resolvedSchemaRows(QStringLiteral("component/%1").arg(componentTypeId));
}

QVariantList PropertySchemaRegistry::connectionSchema(const QString &connectionTypeId) const
{
    return resolvedSchemaRows(QStringLiteral("connection/%1").arg(connectionTypeId));
}

bool PropertySchemaRegistry::componentSchemaTyped(
    const QString &componentTypeId,
    cme::publicapi::v1::PropertySchemaResponse *out,
    QString *error) const
{
    return schemaForTargetTyped(QStringLiteral("component/%1").arg(componentTypeId), out, error);
}

bool PropertySchemaRegistry::connectionSchemaTyped(
    const QString &connectionTypeId,
    cme::publicapi::v1::PropertySchemaResponse *out,
    QString *error) const
{
    return schemaForTargetTyped(QStringLiteral("connection/%1").arg(connectionTypeId), out, error);
}

QVariantMap PropertySchemaRegistry::normalizeFieldRow(const QVariantMap &raw, const QString &targetId)
{
    const cme::runtime::SchemaFieldDefinition field =
        cme::runtime::SchemaFieldLegacyAdapter::fromLegacyRow(raw, targetId);
    return cme::runtime::SchemaFieldLegacyAdapter::toNormalizedRow(field);
}

QVariantList PropertySchemaRegistry::normalizeRows(const QVariantList &rows, const QString &targetId)
{
    QVariantList normalized;
    normalized.reserve(rows.size());

    for (const QVariant &value : rows) {
        const QVariantMap row = value.toMap();
        if (row.isEmpty())
            continue;
        normalized.append(normalizeFieldRow(row, targetId));
    }

    return normalized;
}

QVariantList PropertySchemaRegistry::sectionizeRows(const QVariantList &rows)
{
    struct IndexedRow {
        int index;
        QVariantMap row;
    };

    QHash<QString, QList<IndexedRow>> bySection;
    QStringList sectionOrder;

    int rowIndex = 0;
    for (const QVariant &value : rows) {
        const QVariantMap row = value.toMap();
        QString section = row.value(QStringLiteral("section")).toString().trimmed();
        if (section.isEmpty())
            section = QStringLiteral("General");

        if (!bySection.contains(section))
            sectionOrder.append(section);

        bySection[section].append({ rowIndex, row });
        ++rowIndex;
    }

    QVariantList sections;
    for (const QString &section : sectionOrder) {
        QList<IndexedRow> fields = bySection.value(section);
        std::sort(fields.begin(), fields.end(), [](const IndexedRow &a, const IndexedRow &b) {
            const int orderA = a.row.value(QStringLiteral("order")).toInt();
            const int orderB = b.row.value(QStringLiteral("order")).toInt();
            if (orderA != orderB)
                return orderA < orderB;
            return a.index < b.index;
        });

        QVariantList rowsInSection;
        rowsInSection.reserve(fields.size());
        for (const IndexedRow &entry : fields)
            rowsInSection.append(entry.row);

        sections.append(QVariantMap{
            { QStringLiteral("id"), section.toLower() },
            { QStringLiteral("title"), section },
            { QStringLiteral("fields"), rowsInSection }
        });
    }

    return sections;
}

QVector<cme::runtime::SchemaSectionDefinition> PropertySchemaRegistry::sectionizeTypedRows(
    const QVariantList &rows,
    const QString &targetId)
{
    struct IndexedField {
        int index;
        cme::runtime::SchemaFieldDefinition field;
    };

    QHash<QString, QVector<IndexedField>> fieldsBySection;
    QStringList sectionOrder;
    int rowIndex = 0;

    for (const QVariant &value : rows) {
        const QVariantMap row = value.toMap();
        if (row.isEmpty())
            continue;

        cme::runtime::SchemaFieldDefinition field =
            cme::runtime::SchemaFieldLegacyAdapter::fromLegacyRow(row, targetId);
        QString section = field.section.trimmed();
        if (section.isEmpty())
            section = QStringLiteral("General");

        if (!fieldsBySection.contains(section))
            sectionOrder.append(section);
        fieldsBySection[section].append({ rowIndex, field });
        ++rowIndex;
    }

    QVector<cme::runtime::SchemaSectionDefinition> sections;
    sections.reserve(sectionOrder.size());

    for (const QString &sectionName : sectionOrder) {
        QVector<IndexedField> indexedFields = fieldsBySection.value(sectionName);
        std::sort(indexedFields.begin(), indexedFields.end(), [](const IndexedField &a, const IndexedField &b) {
            if (a.field.order != b.field.order)
                return a.field.order < b.field.order;
            return a.index < b.index;
        });

        cme::runtime::SchemaSectionDefinition section;
        section.id = sectionName.toLower();
        section.title = sectionName;
        section.fields.reserve(indexedFields.size());
        for (const IndexedField &indexedField : indexedFields)
            section.fields.append(indexedField.field);
        sections.append(section);
    }

    return sections;
}

QVariantList PropertySchemaRegistry::fallbackComponentRows()
{
    return {
        makeFallbackField(QStringLiteral("ID"), QStringLiteral("id"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 0),
        makeFallbackField(QStringLiteral("Title"), QStringLiteral("title"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 1),
        makeFallbackField(QStringLiteral("Content"), QStringLiteral("content"), QStringLiteral("textarea"), QString(), QStringLiteral("Identity"), 2),
        makeFallbackField(QStringLiteral("Icon"), QStringLiteral("icon"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 3),
        makeFallbackField(QStringLiteral("Type"), QStringLiteral("type"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 4),
        makeFallbackField(QStringLiteral("Color"), QStringLiteral("color"), QStringLiteral("textfield"), QStringLiteral("#4fc3f7"), QStringLiteral("Appearance"), 10),
        makeFallbackField(QStringLiteral("Shape"), QStringLiteral("shape"), QStringLiteral("dropdown"), QStringLiteral("rounded"), QStringLiteral("Appearance"), 11),
        makeFallbackField(QStringLiteral("X"), QStringLiteral("x"), QStringLiteral("spinbox"), 0, QStringLiteral("Geometry"), 20),
        makeFallbackField(QStringLiteral("Y"), QStringLiteral("y"), QStringLiteral("spinbox"), 0, QStringLiteral("Geometry"), 21),
        makeFallbackField(QStringLiteral("Width"), QStringLiteral("width"), QStringLiteral("spinbox"), 96, QStringLiteral("Geometry"), 22),
        makeFallbackField(QStringLiteral("Height"), QStringLiteral("height"), QStringLiteral("spinbox"), 96, QStringLiteral("Geometry"), 23)
    };
}

QVariantList PropertySchemaRegistry::fallbackConnectionRows()
{
    return {
        makeFallbackField(QStringLiteral("ID"), QStringLiteral("id"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 0),
        makeFallbackField(QStringLiteral("Source"), QStringLiteral("sourceId"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 1),
        makeFallbackField(QStringLiteral("Target"), QStringLiteral("targetId"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 2),
        makeFallbackField(QStringLiteral("Label"), QStringLiteral("label"), QStringLiteral("textfield"), QString(), QStringLiteral("Identity"), 3),
        makeFallbackField(QStringLiteral("Source Side"), QStringLiteral("sourceSide"), QStringLiteral("dropdown"), -1, QStringLiteral("Routing"), 10),
        makeFallbackField(QStringLiteral("Target Side"), QStringLiteral("targetSide"), QStringLiteral("dropdown"), -1, QStringLiteral("Routing"), 11)
    };
}

QVariantList PropertySchemaRegistry::resolvedSchemaRows(const QString &targetId) const
{
    const QVariantList direct = m_rowsByTarget.value(targetId);
    if (!direct.isEmpty())
        return direct;

    if (targetId.startsWith(QStringLiteral("component/")))
        return fallbackComponentRows();
    if (targetId.startsWith(QStringLiteral("connection/")))
        return fallbackConnectionRows();

    return {};
}
