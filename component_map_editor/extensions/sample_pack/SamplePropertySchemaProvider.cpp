#include "SamplePropertySchemaProvider.h"

namespace {

QVariantMap entry(const char *key, const char *type, const char *title,
                  bool required, const QVariant &defaultValue,
                  const char *editor)
{
    return QVariantMap{
        { QStringLiteral("key"),          QString::fromLatin1(key) },
        { QStringLiteral("type"),         QString::fromLatin1(type) },
        { QStringLiteral("title"),        QString::fromLatin1(title) },
        { QStringLiteral("required"),     required },
        { QStringLiteral("defaultValue"), defaultValue },
        { QStringLiteral("editor"),       QString::fromLatin1(editor) }
    };
}

QVariantMap withSection(QVariantMap row,
                        const char *section,
                        int order,
                        const QString &hint = QString(),
                        const QVariantMap &validation = {},
                        const QVariantMap &visibleWhen = {},
                        const QVariantList &options = {})
{
    row.insert(QStringLiteral("section"), QString::fromLatin1(section));
    row.insert(QStringLiteral("order"), order);
    if (!hint.isEmpty())
        row.insert(QStringLiteral("hint"), hint);
    if (!validation.isEmpty())
        row.insert(QStringLiteral("validation"), validation);
    if (!visibleWhen.isEmpty())
        row.insert(QStringLiteral("visibleWhen"), visibleWhen);
    if (!options.isEmpty())
        row.insert(QStringLiteral("options"), options);
    return row;
}

} // namespace

QString SamplePropertySchemaProvider::providerId() const
{
    return QStringLiteral("sample.workflow.propertySchema");
}

QStringList SamplePropertySchemaProvider::schemaTargets() const
{
    return {
        QStringLiteral("component/start"),
        QStringLiteral("component/task"),
        QStringLiteral("component/decision"),
        QStringLiteral("component/end"),
        QStringLiteral("connection/flow")
    };
}

QVariantList SamplePropertySchemaProvider::propertySchema(const QString &targetId) const
{
    if (targetId == QStringLiteral("component/start") ||
        targetId == QStringLiteral("component/end")) {
        return {
            withSection(entry("title", "string", "Title", true, QString(), "textfield"),
                        "Identity", 1,
                        QStringLiteral("Human-friendly label shown on the graph.")),
            withSection(entry("icon", "string", "Icon", false, QString(), "textfield"),
                        "Appearance", 10,
                        QStringLiteral("FontAwesome icon key, for example 'play' or 'stop'.")),
            withSection(entry("color", "string", "Color", true, QStringLiteral("#66bb6a"), "textfield"),
                        "Appearance", 11),
            withSection(entry("shape", "enum", "Shape", true, QStringLiteral("rounded"), "dropdown"),
                        "Appearance", 12,
                        QString(),
                        {},
                        {},
                        QVariantList{ QStringLiteral("rounded"), QStringLiteral("rectangle") })
        };
    }

    if (targetId == QStringLiteral("component/task")) {
        return {
            withSection(entry("title", "string", "Title", true, QString(), "textfield"),
                        "Identity", 1,
                        QStringLiteral("Display title used in the node body.")),
            withSection(entry("description", "string", "Description", false, QString(), "textarea"),
                        "Behavior", 20,
                        QStringLiteral("Optional implementation notes for this task.")),
            withSection(entry("priority", "enum", "Priority", true, QStringLiteral("normal"), "dropdown"),
                        "Behavior", 21,
                        QStringLiteral("Higher priority tasks can be highlighted by business rules."),
                        {},
                        {},
                        QVariantList{
                            QVariantMap{{QStringLiteral("text"), QStringLiteral("Low")}, {QStringLiteral("value"), QStringLiteral("low")}},
                            QVariantMap{{QStringLiteral("text"), QStringLiteral("Normal")}, {QStringLiteral("value"), QStringLiteral("normal")}},
                            QVariantMap{{QStringLiteral("text"), QStringLiteral("High")}, {QStringLiteral("value"), QStringLiteral("high")}}
                        }),
            withSection(entry("x", "number", "X", true, 0, "spinbox"),
                        "Layout", 40,
                        QString(),
                        QVariantMap{{QStringLiteral("min"), -50000}, {QStringLiteral("max"), 50000}}),
            withSection(entry("y", "number", "Y", true, 0, "spinbox"),
                        "Layout", 41,
                        QString(),
                        QVariantMap{{QStringLiteral("min"), -50000}, {QStringLiteral("max"), 50000}}),
            withSection(entry("width", "number", "Width", true, 160, "spinbox"),
                        "Layout", 42,
                        QString(),
                        QVariantMap{{QStringLiteral("min"), 40}, {QStringLiteral("max"), 1200}}),
            withSection(entry("height", "number", "Height", true, 96, "spinbox"),
                        "Layout", 43,
                        QString(),
                        QVariantMap{{QStringLiteral("min"), 40}, {QStringLiteral("max"), 1200}})
        };
    }

    if (targetId == QStringLiteral("component/decision")) {
        return {
            withSection(entry("title", "string", "Title", true, QString(), "textfield"),
                        "Identity", 1),
            withSection(entry("condition", "string", "Condition", true, QString(), "textfield"),
                        "Behavior", 20,
                        QStringLiteral("Expression evaluated by the business runtime."),
                        QVariantMap{{QStringLiteral("requiredMessage"), QStringLiteral("Condition is required.")}}),
            withSection(entry("description", "string", "Description", false, QString(), "textarea"),
                        "Behavior", 21,
                        QStringLiteral("Shown only when a condition exists."),
                        {},
                        QVariantMap{{QStringLiteral("property"), QStringLiteral("condition")}, {QStringLiteral("truthy"), true}})
        };
    }

    if (targetId == QStringLiteral("connection/flow")) {
        const QVariantList sideOptions{
            QVariantMap{{QStringLiteral("text"), QStringLiteral("Auto")},   {QStringLiteral("value"), -1}},
            QVariantMap{{QStringLiteral("text"), QStringLiteral("Top")},    {QStringLiteral("value"),  0}},
            QVariantMap{{QStringLiteral("text"), QStringLiteral("Right")},  {QStringLiteral("value"),  1}},
            QVariantMap{{QStringLiteral("text"), QStringLiteral("Bottom")}, {QStringLiteral("value"),  2}},
            QVariantMap{{QStringLiteral("text"), QStringLiteral("Left")},   {QStringLiteral("value"),  3}}
        };
        return {
            withSection(entry("label", "string", "Label", false, QString(), "textfield"),
                        "Identity", 1),
            withSection(entry("sourceSide", "enum", "Source Side", true, -1, "dropdown"),
                        "Routing", 20, QString(), {}, {}, sideOptions),
            withSection(entry("targetSide", "enum", "Target Side", true, -1, "dropdown"),
                        "Routing", 21, QString(), {}, {}, sideOptions)
        };
    }

    return {};
}
