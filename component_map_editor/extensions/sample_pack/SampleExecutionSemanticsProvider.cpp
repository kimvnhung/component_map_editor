#include "SampleExecutionSemanticsProvider.h"

#include <QDebug>

QString SampleExecutionSemanticsProvider::providerId() const
{
    return QStringLiteral("sample.workflow.execution");
}

QStringList SampleExecutionSemanticsProvider::supportedComponentTypes() const
{
    return {
        QStringLiteral("start"),
        QStringLiteral("process"),
        QStringLiteral("stop")
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

    auto resolveNumber = [](const QVariant &value, double fallback) {
        bool ok = false;
        const double parsed = value.toDouble(&ok);
        return ok ? parsed : fallback;
    };

    if (componentType == QStringLiteral("start")) {
        state.insert(QStringLiteral("started"), true);
        const double inputNumber = resolveNumber(state.value(QStringLiteral("inputNumber")),
                                                 resolveNumber(componentSnapshot.value(QStringLiteral("inputNumber")), 0.0));
        state.insert(QStringLiteral("workingNumber"), inputNumber);
        state.insert(QStringLiteral("inputNumber"), inputNumber);
        if (trace)
            trace->insert(QStringLiteral("note"), QStringLiteral("Start seeded workingNumber from inputNumber."));
    } else if (componentType == QStringLiteral("process")) {
        const double current = resolveNumber(state.value(QStringLiteral("workingNumber")), 0.0);
        const double addValue = resolveNumber(componentSnapshot.value(QStringLiteral("addValue")), 9.0);
        const double result = current + addValue;
        state.insert(QStringLiteral("workingNumber"), result);
        state.insert(QStringLiteral("lastProcessAddValue"), addValue);
        if (trace) {
            trace->insert(QStringLiteral("input"), current);
            trace->insert(QStringLiteral("addValue"), addValue);
            trace->insert(QStringLiteral("result"), result);
        }
    } else if (componentType == QStringLiteral("stop")) {
        const double finalResult = resolveNumber(state.value(QStringLiteral("workingNumber")), 0.0);
        state.insert(QStringLiteral("finalResult"), finalResult);
        state.insert(QStringLiteral("completed"), true);
        qInfo().noquote() << QStringLiteral("[SampleWorkflow] Stop '%1' result: %2")
                                 .arg(componentId)
                                 .arg(finalResult);
        if (trace)
            trace->insert(QStringLiteral("result"), finalResult);
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
