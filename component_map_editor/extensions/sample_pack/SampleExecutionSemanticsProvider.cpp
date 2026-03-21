#include "SampleExecutionSemanticsProvider.h"

QString SampleExecutionSemanticsProvider::providerId() const
{
    return QStringLiteral("sample.workflow.execution");
}

QStringList SampleExecutionSemanticsProvider::supportedComponentTypes() const
{
    return {
        QStringLiteral("start"),
        QStringLiteral("task"),
        QStringLiteral("decision"),
        QStringLiteral("end")
    };
}

bool SampleExecutionSemanticsProvider::executeComponent(const QString &componentType,
                                                        const QString &componentId,
                                                        const QVariantMap &componentSnapshot,
                                                        const QVariantMap &inputState,
                                                        QVariantMap *outputState,
                                                        QVariantMap *trace,
                                                        QString *error) const
{
    Q_UNUSED(error)

    QVariantMap state = inputState;

    if (componentType == QStringLiteral("start")) {
        state.insert(QStringLiteral("started"), true);
    } else if (componentType == QStringLiteral("task")) {
        const int executedTasks = state.value(QStringLiteral("executedTaskCount"), 0).toInt() + 1;
        state.insert(QStringLiteral("executedTaskCount"), executedTasks);
    } else if (componentType == QStringLiteral("decision")) {
        const QString condition = componentSnapshot.value(QStringLiteral("condition")).toString();
        state.insert(QStringLiteral("lastDecisionCondition"), condition);
        state.insert(QStringLiteral("lastDecisionResult"), condition != QStringLiteral("false"));
    } else if (componentType == QStringLiteral("end")) {
        state.insert(QStringLiteral("completed"), true);
    }

    state.insert(QStringLiteral("lastExecutedComponentId"), componentId);

    if (outputState)
        *outputState = state;

    if (trace) {
        trace->insert(QStringLiteral("provider"), providerId());
        trace->insert(QStringLiteral("componentType"), componentType);
        trace->insert(QStringLiteral("componentId"), componentId);
    }

    return true;
}
