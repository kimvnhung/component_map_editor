#include "customizestopexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeStopExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.stop");
}

QStringList CustomizeStopExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
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
    qDebug() << "context" << context;
    QVariantMap out = context;
    const int endNumber = static_cast<int>(customize::executors::resolveNumber(context,
                                                                                componentSnapshot,
                                                                                QStringLiteral("endNumber"),
                                                                                QStringLiteral("endNumber"),
                                                                                0.0));
    out.insert(QStringLiteral("endNumber"), endNumber);
    out.insert(QStringLiteral("completed"), true);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2 - Stopping execution with endNumber: %3")
                             .arg(componentType, componentId)
                             .arg(endNumber);

    if (outputPayload)
        *outputPayload = out;
    if (trace)
        *trace = customize::executors::makeTracePayload(componentType, componentId, context, out);

    return true;
}