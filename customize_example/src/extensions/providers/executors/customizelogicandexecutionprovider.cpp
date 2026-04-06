#include "customizelogicandexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeLogicAndExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.logic.and");
}

QStringList CustomizeLogicAndExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

bool CustomizeLogicAndExecutionProvider::executeComponent(const QString &componentType,
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

bool CustomizeLogicAndExecutionProvider::executeComponentV2(
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