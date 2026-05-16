#include "customizemodexecutionprovider.h"

#include "customizeexecutioncommon.h"

#include <base_log.h>

QString CustomizeModExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.math.mod");
}

QStringList CustomizeModExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeModExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("result"), QStringLiteral("error") };
}

bool CustomizeModExecutionProvider::executeComponent(
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
                                                                QStringLiteral("result"));
    const QString errorKey = customize::executors::resolveText(componentSnapshot,
                                                               QStringLiteral("errorKey"),
                                                               QStringLiteral("error"));

    bool okA = false;
    bool okB = false;
    const double a = customize::executors::resolveReferencedNumber(incomingTokens,
                                                                   componentSnapshot,
                                                                   QStringLiteral("inputARef"),
                                                                   QStringLiteral("a"),
                                                                   1.0,
                                                                   &okA);
    const double b = customize::executors::resolveReferencedNumber(incomingTokens,
                                                                   componentSnapshot,
                                                                   QStringLiteral("inputBRef"),
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

    if (qFuzzyIsNull(b)) {
        return customize::executors::failExecution(componentType,
                                                   componentId,
                                                   context,
                                                   out,
                                                   errorKey,
                                                   QStringLiteral("Modulo by zero is not allowed."),
                                                   outputPayload,
                                                   trace,
                                                   error);
    }

    out.insert(outputKey, std::fmod(a, b));

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    INFOF("[Trace][{}] {}", componentType.toStdString(), componentId.toStdString());

    return true;
}