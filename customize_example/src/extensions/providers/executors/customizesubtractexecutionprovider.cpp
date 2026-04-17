#include "customizesubtractexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeSubtractExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.math.subtract");
}

QStringList CustomizeSubtractExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeSubtractExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("difference"), QStringLiteral("error") };
}
bool CustomizeSubtractExecutionProvider::executeComponent(
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

    const QString outputKey = customize::executors::resolveText(componentSnapshot,
                                                                QStringLiteral("outputKey"),
                                                                QStringLiteral("difference"));
    const QString errorKey = customize::executors::resolveText(componentSnapshot,
                                                               QStringLiteral("errorKey"),
                                                               QStringLiteral("error"));

    bool okA = false;
    bool okB = false;
    const double a = customize::executors::resolveReferencedNumber(incomingTokens,
                                                                   componentSnapshot,
                                                                   QStringLiteral("inputARef"),
                                                                   QStringLiteral("a"),
                                                                   0.0,
                                                                   &okA);
    const double b = customize::executors::resolveReferencedNumber(incomingTokens,
                                                                   componentSnapshot,
                                                                   QStringLiteral("inputBRef"),
                                                                   QStringLiteral("b"),
                                                                   0.0,
                                                                   &okB);

    if (!okA || !okB) {
        const QString msg = QStringLiteral("Invalid numeric input for operation '%1'.").arg(componentType);
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

    out.insert(outputKey, a - b);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2")
                             .arg(componentType, componentId);

    return true;
}
