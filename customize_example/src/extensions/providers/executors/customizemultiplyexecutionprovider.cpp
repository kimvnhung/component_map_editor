#include "customizemultiplyexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeMultiplyExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.math.multiply");
}

QStringList CustomizeMultiplyExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}
bool CustomizeMultiplyExecutionProvider::executeComponent(
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
                                                                QStringLiteral("product"));
    const QString errorKey = customize::executors::resolveText(componentSnapshot,
                                                               QStringLiteral("errorKey"),
                                                               QStringLiteral("error"));

    bool okA = false;
    bool okB = false;
    const double a = customize::executors::resolveSelectedNumber(incomingTokens,
                                                                 context,
                                                                 componentSnapshot,
                                                                 QStringLiteral("inputARef"),
                                                                 QStringLiteral("inputAKey"),
                                                                 QStringLiteral("a"),
                                                                 1.0,
                                                                 &okA);
    const double b = customize::executors::resolveSelectedNumber(incomingTokens,
                                                                 context,
                                                                 componentSnapshot,
                                                                 QStringLiteral("inputBRef"),
                                                                 QStringLiteral("inputBKey"),
                                                                 QStringLiteral("b"),
                                                                 1.0,
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

    out.insert(outputKey, a * b);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2")
                             .arg(componentType, componentId);

    return true;
}
