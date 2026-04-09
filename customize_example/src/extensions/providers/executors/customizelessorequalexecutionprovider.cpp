#include "customizelessorequalexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeLessOrEqualExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.math.less_or_equal");
}

QStringList CustomizeLessOrEqualExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}
bool CustomizeLessOrEqualExecutionProvider::executeComponent(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    const QVariantMap context = customize::executors::mergeIncomingTokens(incomingTokens);

    qInfo().noquote() << QStringLiteral("[Trace][%1] %2 executing component '%3'")
                             .arg(providerId(), componentType, componentId);

    return true;
}
