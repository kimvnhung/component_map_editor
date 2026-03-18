#include "PropertySchemaRegistry.h"

#include <algorithm>

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

QVariantList PropertySchemaRegistry::sectionedSchemaForTarget(const QString &targetId) const
{
    return sectionizeRows(resolvedSchemaRows(targetId));
}

QVariantList PropertySchemaRegistry::componentSchema(const QString &componentTypeId) const
{
    return resolvedSchemaRows(QStringLiteral("component/%1").arg(componentTypeId));
}

QVariantList PropertySchemaRegistry::connectionSchema(const QString &connectionTypeId) const
{
    return resolvedSchemaRows(QStringLiteral("connection/%1").arg(connectionTypeId));
}

QVariantMap PropertySchemaRegistry::normalizeFieldRow(const QVariantMap &raw, const QString &targetId)
{
    QVariantMap out;
    const QString key = raw.value(QStringLiteral("key")).toString().trimmed();
    const QString title = raw.value(QStringLiteral("title")).toString().trimmed();

    const QString editorLegacy = raw.value(QStringLiteral("editor")).toString().trimmed();
    const QString widget = raw.value(QStringLiteral("widget")).toString().trimmed();
    const QString resolvedWidget = widget.isEmpty() ? editorLegacy : widget;

    QString error;
    if (key.isEmpty()) {
        error = QStringLiteral("Schema field in target '%1' is missing key.").arg(targetId);
    } else if (resolvedWidget.isEmpty()) {
        error = QStringLiteral("Schema field '%1' in target '%2' is missing widget/editor.")
                    .arg(key, targetId);
    }

    out.insert(QStringLiteral("key"), key);
    out.insert(QStringLiteral("title"), title.isEmpty() ? key : title);
    out.insert(QStringLiteral("type"), raw.value(QStringLiteral("type")).toString());
    out.insert(QStringLiteral("widget"), error.isEmpty() ? resolvedWidget : QStringLiteral("schema_error"));
    out.insert(QStringLiteral("required"), raw.value(QStringLiteral("required")).toBool());
    out.insert(QStringLiteral("defaultValue"), raw.value(QStringLiteral("defaultValue")));
    out.insert(QStringLiteral("section"), raw.value(QStringLiteral("section")).toString());
    out.insert(QStringLiteral("order"), raw.value(QStringLiteral("order")).toInt());
    out.insert(QStringLiteral("hint"), raw.value(QStringLiteral("hint")).toString());
    out.insert(QStringLiteral("placeholder"), raw.value(QStringLiteral("placeholder")).toString());
    out.insert(QStringLiteral("options"), raw.value(QStringLiteral("options")).toList());
    out.insert(QStringLiteral("visibleWhen"), raw.value(QStringLiteral("visibleWhen")).toMap());
    out.insert(QStringLiteral("validation"), raw.value(QStringLiteral("validation")).toMap());
    out.insert(QStringLiteral("valid"), error.isEmpty());
    out.insert(QStringLiteral("schemaError"), error);

    return out;
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
        makeFallbackField(QStringLiteral("Source Side"), QStringLiteral("sourceSide"), QStringLiteral("dropdown"), 0, QStringLiteral("Routing"), 10),
        makeFallbackField(QStringLiteral("Target Side"), QStringLiteral("targetSide"), QStringLiteral("dropdown"), 0, QStringLiteral("Routing"), 11)
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
