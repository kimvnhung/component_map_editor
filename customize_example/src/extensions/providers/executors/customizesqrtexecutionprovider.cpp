#include "customizesqrtexecutionprovider.h"

#include <QtMath>

#include "customizeexecutioncommon.h"

#include <base_log.h>

QString CustomizeSqrtExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.math.sqrt");
}

QStringList CustomizeSqrtExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeSqrtExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("sqrtS"), QStringLiteral("error") };
}

bool CustomizeSqrtExecutionProvider::executeComponent(
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
                                                                QStringLiteral("sqrtS"));
    const QString errorKey = customize::executors::resolveText(componentSnapshot,
                                                               QStringLiteral("errorKey"),
                                                               QStringLiteral("error"));

    bool ok = false;
    const double radicand = customize::executors::resolveReferencedNumber(incomingTokens,
                                                                          componentSnapshot,
                                                                          QStringLiteral("inputRef"),
                                                                          QStringLiteral("S"),
                                                                          0.0,
                                                                          &ok);
    if (!ok) {
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

    if (radicand < 0.0) {
        return customize::executors::failExecution(componentType,
                                                   componentId,
                                                   context,
                                                   out,
                                                   errorKey,
                                                   QStringLiteral("Square root is undefined for negative values."),
                                                   outputPayload,
                                                   trace,
                                                   error);
    }

    out.insert(outputKey, qSqrt(radicand));

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    INFOF("[Trace][{}] {}", componentType.toStdString(), componentId.toStdString());

    return true;
}