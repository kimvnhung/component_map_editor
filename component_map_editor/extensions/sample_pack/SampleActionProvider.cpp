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

bool SampleActionProvider::invokeAction(const QString &actionId,
                                        const QVariantMap &context,
                                        QVariantMap *commandRequest,
                                        QString *error) const
{
    if (actionId != QLatin1String(ActionSetTaskPriority)) {
        if (error)
            *error = QStringLiteral("Unknown action id: %1").arg(actionId);
        return false;
    }

    const QString componentId  = context.value(QStringLiteral("componentId")).toString();
    const QString newPriority  = context.value(QStringLiteral("newPriority")).toString();

    if (componentId.isEmpty()) {
        if (error)
            *error = QStringLiteral("setTaskPriority requires context key 'componentId'.");
        return false;
    }

    static const QStringList validPriorities = {
        QStringLiteral("low"), QStringLiteral("normal"), QStringLiteral("high")
    };
    if (!validPriorities.contains(newPriority)) {
        if (error)
            *error = QStringLiteral("setTaskPriority requires 'newPriority' to be one of: low, normal, high.");
        return false;
    }

    if (commandRequest) {
        *commandRequest = QVariantMap{
            { QStringLiteral("command"),      QStringLiteral("setComponentProperty") },
            { QStringLiteral("id"),           componentId },
            { QStringLiteral("property"),     QStringLiteral("priority") },
            { QStringLiteral("value"),        newPriority },
            { QStringLiteral("commandType"),  QStringLiteral("setComponentProperty") },
            { QStringLiteral("componentId"),  componentId },
            { QStringLiteral("propertyName"), QStringLiteral("priority") },
            { QStringLiteral("newValue"),     newPriority }
        };
    }
    return true;
}
