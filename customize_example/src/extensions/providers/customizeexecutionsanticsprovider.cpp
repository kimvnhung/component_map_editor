#include "customizeexecutionsanticsprovider.h"

QString CustomizeExecutionSemanticsProvider::providerId() const
{
    return QStringLiteral("customize.workflow.execution");
}

QStringList CustomizeExecutionSemanticsProvider::supportedComponentTypes() const
{
    return {
        QString::fromLatin1(TypeStart),
        QString::fromLatin1(TypeStop),
        QString::fromLatin1(TypeLoop),
        QString::fromLatin1(TypeIfElse),
        QString::fromLatin1(TypeAdd),
        QString::fromLatin1(TypeSubtract),
        QString::fromLatin1(TypeMultiply),
        QString::fromLatin1(TypeDivide),
        QString::fromLatin1(TypeErrorHandler),
        QString::fromLatin1(TypeMod),
        QString::fromLatin1(TypeLessThan),
        QString::fromLatin1(TypeLessOrEqual),
        QString::fromLatin1(TypeEqual),
        QString::fromLatin1(TypeLogicAnd)
    };
}

bool CustomizeExecutionSemanticsProvider::executeComponent(
    const QString &componentType,
    const QString &componentId,
    const QVariantMap &componentSnapshot,
    const cme::execution::IncomingTokens &incomingTokens,
    cme::execution::ExecutionPayload *outputPayload,
    QVariantMap *trace,
    QString *error) const
{
    const IExecutionSemanticsProvider *delegate = providerForType(componentType);
    if (!delegate) {
        QVariantMap passthrough;
        QStringList tokenKeys = incomingTokens.keys();
        std::sort(tokenKeys.begin(), tokenKeys.end());
        for (const QString &tokenKey : tokenKeys)
            passthrough.insert(incomingTokens.value(tokenKey));

        if (outputPayload)
            *outputPayload = passthrough;
        if (trace)
            *trace = {
                { QStringLiteral("componentType"), componentType },
                { QStringLiteral("componentId"), componentId },
                { QStringLiteral("inputs"), passthrough },
                { QStringLiteral("outputs"), passthrough },
                { QStringLiteral("provider"), QStringLiteral("default") },
                { QStringLiteral("note"), QStringLiteral("No execution semantics provider registered for component type.") }
            };
        return true;
    }

    return delegate->executeComponent(componentType,
                                        componentId,
                                        componentSnapshot,
                                        incomingTokens,
                                        outputPayload,
                                        trace,
                                        error);
}

const IExecutionSemanticsProvider *CustomizeExecutionSemanticsProvider::providerForType(const QString &componentType) const
{
    if (componentType == QLatin1String(TypeStart))
        return &m_startProvider;
    if (componentType == QLatin1String(TypeStop))
        return &m_stopProvider;
    if (componentType == QLatin1String(TypeAdd))
        return &m_addProvider;
    if (componentType == QLatin1String(TypeSubtract))
        return &m_subtractProvider;
    if (componentType == QLatin1String(TypeMultiply))
        return &m_multiplyProvider;
    if (componentType == QLatin1String(TypeDivide))
        return &m_divideProvider;
    if (componentType == QLatin1String(TypeIfElse))
        return &m_ifElseProvider;
    if (componentType == QLatin1String(TypeLoop))
        return &m_loopProvider;
    if (componentType == QLatin1String(TypeErrorHandler))
        return &m_errorHandlerProvider;
    return nullptr;
}
