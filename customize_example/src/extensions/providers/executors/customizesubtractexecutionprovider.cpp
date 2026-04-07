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

bool CustomizeSubtractExecutionProvider::executeComponent(const QString &componentType,
                                                          const QString &componentId,
                                                          const QVariantMap &componentSnapshot,
                                                          const QVariantMap &inputState,
                                                          QVariantMap *outputState,
                                                          QVariantMap *trace,
                                                          QString *error) const
{
    return executeComponentV2(componentType,
                              componentId,
                              componentSnapshot,
                              customize::executors::makeLegacyIncomingTokens(inputState),
                              outputState,
                              trace,
                              error);
}

bool CustomizeSubtractExecutionProvider::executeComponentV2(
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

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2")
                             .arg(componentType, componentId);

    return true;
}
