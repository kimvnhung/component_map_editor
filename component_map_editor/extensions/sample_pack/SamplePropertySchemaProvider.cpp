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
            entry("name", "string", "Name", false, QString(), "textfield")
        };
    }

    if (targetId == QStringLiteral("component/task")) {
        return {
            entry("name",        "string", "Name",        true,  QString(),          "textfield"),
            entry("description", "string", "Description", false, QString(),          "textarea"),
            entry("priority",    "enum",   "Priority",    true,  QStringLiteral("normal"), "dropdown")
        };
    }

    if (targetId == QStringLiteral("component/decision")) {
        return {
            entry("name",      "string", "Name",      true, QString(), "textfield"),
            entry("condition", "string", "Condition", true, QString(), "textfield")
        };
    }

    if (targetId == QStringLiteral("connection/flow")) {
        return {
            entry("label", "string", "Label", false, QString(), "textfield")
        };
    }

    return {};
}
