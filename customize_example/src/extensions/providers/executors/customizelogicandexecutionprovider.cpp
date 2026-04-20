#include "customizelogicandexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeLogicAndExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.logic.and");
}

QStringList CustomizeLogicAndExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeLogicAndExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("result"), QStringLiteral("error") };
}

bool CustomizeLogicAndExecutionProvider::executeComponent(
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

    const QString inputAKey = customize::executors::resolveText(componentSnapshot,
                                                                QStringLiteral("inputAKey"),
                                                                QStringLiteral("a"));
    const QString inputBKey = customize::executors::resolveText(componentSnapshot,
                                                                QStringLiteral("inputBKey"),
                                                                QStringLiteral("b"));
    const QString outputKey = customize::executors::resolveText(componentSnapshot,
                                                                QStringLiteral("outputKey"),
                                                                QStringLiteral("result"));
    const QString errorKey = customize::executors::resolveText(componentSnapshot,
                                                               QStringLiteral("errorKey"),
                                                               QStringLiteral("error"));

    bool okA = false;
    bool okB = false;
    const bool a = customize::executors::resolveBool(context, componentSnapshot,
                                                     inputAKey, QStringLiteral("a"), false, &okA);
    const bool b = customize::executors::resolveBool(context, componentSnapshot,
                                                     inputBKey, QStringLiteral("b"), false, &okB);

    if (!okA || !okB) {
        const QString msg = QStringLiteral("Invalid boolean input for operation '%1'.").arg(componentType);
        return customize::executors::failExecution(componentType,
                                                   componentId,
                                                   context,
                                                   out,
                                                   errorKey,
                                                   msg,
                                                   outputPayload,
                                                   trace,
                                                   error);
    }

    out.insert(outputKey, a && b);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2")
                      .arg(componentType, componentId);

    return true;
}