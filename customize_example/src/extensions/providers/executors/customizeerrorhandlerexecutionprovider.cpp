#include "customizeerrorhandlerexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeErrorHandlerExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.system.error_handler");
}

QStringList CustomizeErrorHandlerExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeErrorHandlerExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("error") };
}
bool CustomizeErrorHandlerExecutionProvider::executeComponent(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    Q_UNUSED(error)

    const QVariantMap context = customize::executors::mergeIncomingTokens(incomingTokens);
    QVariantMap out = context;
    const QString errorKey = customize::executors::resolveText(componentSnapshot,
                                                               QStringLiteral("errorKey"),
                                                               QStringLiteral("error"));
    if (!out.contains(errorKey)) {
        out.insert(errorKey,
                   customize::executors::resolveText(componentSnapshot,
                                                    QStringLiteral("message"),
                                                    QStringLiteral("Unhandled workflow error.")));
    }

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    qWarning().noquote() << QStringLiteral("[Trace][%1] %2 handled=%3")
                                .arg(componentType, componentId, out.value(errorKey).toString());
    return true;
}
