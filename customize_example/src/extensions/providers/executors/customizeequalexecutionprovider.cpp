#include "customizeequalexecutionprovider.h"

#include "customizeexecutioncommon.h"

QString CustomizeEqualExecutionProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution");
}

QStringList CustomizeEqualExecutionProvider::supportedComponentTypes() const
{
    return { QString::fromLatin1(TypeId) };
}

bool CustomizeEqualExecutionProvider::executeComponent(const QString &componentType,
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

bool CustomizeEqualExecutionProvider::executeComponentV2(
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

    const QString inputAKey =  customize::executors::resolveText(componentSnapshot, QStringLiteral("inputAKey"), QStringLiteral("a"));
    const QString inputBKey = customize::executors::resolveText(componentSnapshot, QStringLiteral("inputBKey"), QStringLiteral("b"));
    const QString outputKey = customize::executors::resolveText(componentSnapshot, QStringLiteral("outputKey"), QStringLiteral("result"));
    const QString errorKey = customize::executors::resolveText(componentSnapshot, QStringLiteral("errorKey"), QStringLiteral("error"));

    bool okA = false;
    bool okB = false;
    const double a = customize::executors::resolveNumber(context, componentSnapshot, inputAKey, QStringLiteral("a"), 0.0, &okA);
    const double b = customize::executors::resolveNumber(context, componentSnapshot, inputBKey, QStringLiteral("b"), 0.0, &okB);

    if (!okA || !okB) {
        const QString msg = QStringLiteral("Invalid numeric input for operation '%1'.").arg(componentType);
        return customize::executors::failExecution(componentType,
                             componentId,
                             context,
                             out,
                             errorKey,
                             msg,
                             outputPayload,
                             trace,
                             error);
    }


    qInfo().noquote() << QStringLiteral("[Trace][%1] %2 = (%3 == %4)")
                       .arg(componentId)
                       .arg(outputKey)
                       .arg(a)
                       .arg(b);

    return true;
}