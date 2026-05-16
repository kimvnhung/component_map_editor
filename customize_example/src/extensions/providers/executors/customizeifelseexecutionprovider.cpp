#include "customizeifelseexecutionprovider.h"

#include "customizeexecutioncommon.h"

#include <base_log.h>

QString CustomizeIfElseExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.control.ifelse");
}

QStringList CustomizeIfElseExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeIfElseExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("routeTrue"), QStringLiteral("routeFalse"), QStringLiteral("error") };
}

bool CustomizeIfElseExecutionProvider::executeComponent(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    const QVariantMap context = customize::executors::mergeIncomingTokens(incomingTokens);
    QVariantMap out = context;

    const QString conditionKey = customize::executors::resolveText(componentSnapshot,
                                                                   QStringLiteral("conditionKey"),
                                                                   QStringLiteral("condition"));
    const QString trueRouteKey = customize::executors::resolveText(componentSnapshot,
                                                                   QStringLiteral("trueRouteKey"),
                                                                   QStringLiteral("routeTrue"));
    const QString falseRouteKey = customize::executors::resolveText(componentSnapshot,
                                                                    QStringLiteral("falseRouteKey"),
                                                                    QStringLiteral("routeFalse"));

    bool okCondition = false;
    const bool condition = customize::executors::resolveBool(context,
                                                             componentSnapshot,
                                                             conditionKey,
                                                             QStringLiteral("condition"),
                                                             false,
                                                             &okCondition);
    if (!okCondition) {
        const QString msg = QStringLiteral("Invalid ifelse condition value.");
        return customize::executors::failExecution(componentType,
                                                   componentId,
                                                   context,
                                                   out,
                                                   QStringLiteral("error"),
                                                   msg,
                                                   outputPayload,
                                                   trace,
                                                   error);
    }

    out.insert(trueRouteKey, condition);
    out.insert(falseRouteKey, !condition);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    INFOF("[Trace][{}] {}", componentType.toStdString(), componentId.toStdString());

    return true;
}
