#include "customizedivideexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeDivideExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution.math.divide");
}

QStringList CustomizeDivideExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

bool CustomizeDivideExecutionProvider::executeComponent(const QString &componentType,
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

bool CustomizeDivideExecutionProvider::executeComponentV2(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    const QVariantMap context = customize::executors::mergeIncomingTokens(incomingTokens);
    return customize::executors::executeBinaryOperation(customize::executors::BinaryOperation::Divide,
                                                        componentType,
                                                        componentId,
                                                        componentSnapshot,
                                                        context,
                                                        outputPayload,
                                                        trace,
                                                        error);
}
