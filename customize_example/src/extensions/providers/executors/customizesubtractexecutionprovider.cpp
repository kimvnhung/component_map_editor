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
    return customize::executors::executeBinaryOperation(customize::executors::BinaryOperation::Subtract,
                                                        componentType,
                                                        componentId,
                                                        componentSnapshot,
                                                        context,
                                                        outputPayload,
                                                        trace,
                                                        error);
}
