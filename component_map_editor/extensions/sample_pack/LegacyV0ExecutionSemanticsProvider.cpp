#include "LegacyV0ExecutionSemanticsProvider.h"

QString LegacyV0ExecutionSemanticsProvider::providerId() const
{
    return QStringLiteral("sample.workflow.execution.legacy.v0");
}

QStringList LegacyV0ExecutionSemanticsProvider::supportedComponentTypes() const
{
    return {
        QStringLiteral("start"),
        QStringLiteral("task"),
        QStringLiteral("decision"),
        QStringLiteral("end")
    };
}

bool LegacyV0ExecutionSemanticsProvider::executeComponent(const QString &componentType,
                                                          const QString &componentId,
                                                          const QVariantMap &componentSnapshot,
                                                          const QVariantMap &inputState,
                                                          QVariantMap *outputState,
                                                          QString *error) const
{
    Q_UNUSED(componentSnapshot)

    if (!outputState) {
        if (error)
            *error = QStringLiteral("outputState is null");
        return false;
    }

    QVariantMap next = inputState;
    next.insert(QStringLiteral("lastComponentId"), componentId);
    next.insert(QStringLiteral("lastComponentType"), componentType);

    if (componentType == QStringLiteral("start")) {
        next.insert(QStringLiteral("started"), true);
    } else if (componentType == QStringLiteral("task")) {
        const int previous = next.value(QStringLiteral("taskCount"), 0).toInt();
        next.insert(QStringLiteral("taskCount"), previous + 1);
    } else if (componentType == QStringLiteral("decision")) {
        next.insert(QStringLiteral("branch"), QStringLiteral("default"));
    } else if (componentType == QStringLiteral("end")) {
        next.insert(QStringLiteral("completed"), true);
    }

    *outputState = next;
    return true;
}
