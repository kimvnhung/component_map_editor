#include "customizestopexecutionprovider.h"

#include "customizeexecutioncommon.h"

#include <base_log.h>

QString CustomizeStopExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.stop");
}

QStringList CustomizeStopExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

QStringList CustomizeStopExecutionProvider::providedOutputKeys(const QString &) const
{
    return { QStringLiteral("endNumber"), QStringLiteral("completed") };
}

bool CustomizeStopExecutionProvider::executeComponent(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    Q_UNUSED(error);

    const QVariantMap context = customize::executors::mergeIncomingTokens(incomingTokens);
    QVariantMap out = context;
    const int endNumber = static_cast<int>(customize::executors::resolveNumber(context,
                                                                                componentSnapshot,
                                                                                QStringLiteral("endNumber"),
                                                                                QStringLiteral("endNumber"),
                                                                                0.0));
    out.insert(QStringLiteral("endNumber"), endNumber);
    out.insert(QStringLiteral("completed"), true);

    INFOF("[Trace][{}] {} - Stopping execution with endNumber: {}", componentType.toStdString(), componentId.toStdString(), endNumber);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    return true;
}