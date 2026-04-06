#include "customizestartexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeStartExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.start");
}

QStringList CustomizeStartExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

bool CustomizeStartExecutionProvider::executeComponent(const QString &componentType,
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

bool CustomizeStartExecutionProvider::executeComponentV2(
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

    int inputNumber = 0;
    inputNumber = static_cast<int>(customize::executors::resolveNumber(context,
                                                              componentSnapshot,
                                                              QStringLiteral("inputNumber"),
                                                              QStringLiteral("inputNumber"),
                                                              0.0));
    out.insert("inputNumber", inputNumber);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2 - input: %3")
                             .arg(componentType, componentId)
                             .arg(inputNumber);

    return true;
}