#include "customizeloopexecutionprovider.h"

#include "customizeexecutioncommon.h"

#include <base_log.h>

QString CustomizeLoopExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.control.loop");
}

QStringList CustomizeLoopExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeLoopExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("iter"), QStringLiteral("continueLoop"), QStringLiteral("error") };
}

bool CustomizeLoopExecutionProvider::executeComponent(
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

    const QString iterKey = customize::executors::resolveText(componentSnapshot,
                                                              QStringLiteral("iterKey"),
                                                              QStringLiteral("iter"));
    const QString maxIterKey = customize::executors::resolveText(componentSnapshot,
                                                                 QStringLiteral("maxIterKey"),
                                                                 QStringLiteral("maxIter"));
    const QString continueKey = customize::executors::resolveText(componentSnapshot,
                                                                  QStringLiteral("continueKey"),
                                                                  QStringLiteral("continueLoop"));
    const QString conditionKey = customize::executors::resolveText(componentSnapshot,
                                                                   QStringLiteral("conditionKey"),
                                                                   QStringLiteral("condition"));

    bool okIter = false;
    bool okMaxIter = false;
    bool okCondition = false;
    const int iter = static_cast<int>(customize::executors::resolveNumber(
        context, componentSnapshot, iterKey, QStringLiteral("iter"), 0.0, &okIter));
    const int maxIter = static_cast<int>(customize::executors::resolveNumber(
        context, componentSnapshot, maxIterKey, QStringLiteral("maxIter"), 10.0, &okMaxIter));
    const bool condition = customize::executors::resolveBool(context,
                                                             componentSnapshot,
                                                             conditionKey,
                                                             QStringLiteral("condition"),
                                                             true,
                                                             &okCondition);

    if (!okIter || !okMaxIter || !okCondition || maxIter <= 0) {
        const QString msg = QStringLiteral("Invalid loop inputs (iter/maxIter/condition).");
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

    const int nextIter = iter + 1;
    const bool shouldContinue = condition && (nextIter < maxIter);
    out.insert(iterKey, nextIter);
    out.insert(continueKey, shouldContinue);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    INFOF("[Trace][{}] {}", componentType.toStdString(), componentId.toStdString());

    return true;
}
