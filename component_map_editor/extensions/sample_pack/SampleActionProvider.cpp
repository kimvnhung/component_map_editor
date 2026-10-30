#include "SampleActionProvider.h"

QString SampleActionProvider::providerId() const
{
    return QStringLiteral("sample.workflow.actions");
}

QStringList SampleActionProvider::actionIds() const
{
    return { QString::fromLatin1(ActionSetTaskPriority) };
}

QVariantMap SampleActionProvider::actionDescriptor(const QString &actionId) const
{
    if (actionId == QLatin1String(ActionSetTaskPriority)) {
        return QVariantMap{
            { QStringLiteral("id"),       QString::fromLatin1(ActionSetTaskPriority) },
            { QStringLiteral("title"),    QStringLiteral("Set Task Priority") },
            { QStringLiteral("category"), QStringLiteral("task") },
            { QStringLiteral("icon"),     QStringLiteral("flag") },
            { QStringLiteral("enabled"),  true }
        };
    }
    return {};
}

