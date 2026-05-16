#include "customizestartexecutionprovider.h"

#include "customizeexecutioncommon.h"

#include <base_log.h>

QString CustomizeStartExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.start");
}

QStringList CustomizeStartExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeStartExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("inputNumber") };
}

bool CustomizeStartExecutionProvider::executeComponent(
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
    out.insert(QStringLiteral("inputNumber"), inputNumber);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    INFOF("[Trace][{}] {} - input: {}", componentType.toStdString(), componentId.toStdString(), inputNumber);

    return true;
}