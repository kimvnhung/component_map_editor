#include "customizeexecutionsanticsprovider.h"

namespace {

bool isRefOnlyMathType(const QString &componentType)
{
    return componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeAdd)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeSubtract)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeMultiply)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeDivide)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeMod)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeLessThan)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeLessOrEqual)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeEqual);
}

QString inferUniqueIncomingReference(const cme::execution::IncomingTokens &incomingTokens,
                                    const QString &fieldKey)
{
    if (fieldKey.trimmed().isEmpty())
        return {};

    QString matchedTokenId;
    QStringList tokenIds = incomingTokens.keys();
    std::sort(tokenIds.begin(), tokenIds.end());
    for (const QString &tokenId : tokenIds) {
        if (!incomingTokens.value(tokenId).contains(fieldKey))
            continue;

        if (!matchedTokenId.isEmpty())
            return {};

        matchedTokenId = tokenId;
    }

    if (matchedTokenId.isEmpty())
        return {};
    return QStringLiteral("%1::%2").arg(matchedTokenId, fieldKey);
}

void normalizeLegacyOperandReference(QVariantMap *componentSnapshot,
                                     const cme::execution::IncomingTokens &incomingTokens,
                                     const QString &refProperty,
                                     const QString &keyProperty)
{
    if (!componentSnapshot)
        return;

    const QString ref = componentSnapshot->value(refProperty).toString().trimmed();
    if (!ref.isEmpty()) {
        componentSnapshot->remove(keyProperty);
        return;
    }

    const QString key = componentSnapshot->value(keyProperty).toString().trimmed();
    if (key.isEmpty())
        return;

    const QString inferredRef = inferUniqueIncomingReference(incomingTokens, key);
    if (!inferredRef.isEmpty())
        componentSnapshot->insert(refProperty, inferredRef);
    componentSnapshot->remove(keyProperty);
}

QVariantMap normalizeComponentSnapshot(const QString &componentType,
                                      const QVariantMap &componentSnapshot,
                                      const cme::execution::IncomingTokens &incomingTokens)
{
    if (!isRefOnlyMathType(componentType))
        return componentSnapshot;

    QVariantMap normalized = componentSnapshot;
    normalizeLegacyOperandReference(&normalized, incomingTokens,
                                    QStringLiteral("inputARef"),
                                    QStringLiteral("inputAKey"));
    normalizeLegacyOperandReference(&normalized, incomingTokens,
                                    QStringLiteral("inputBRef"),
                                    QStringLiteral("inputBKey"));
    return normalized;
}

} // namespace

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

    const QVariantMap normalizedSnapshot = normalizeComponentSnapshot(componentType,
                                                                     componentSnapshot,
                                                                     incomingTokens);

    return delegate->executeComponent(componentType,
                                      componentId,
                                      normalizedSnapshot,
                                      incomingTokens,
                                      outputPayload,
                                      trace,
                                      error);
}

QStringList CustomizeExecutionSemanticsProvider::providedOutputKeys(const QString &componentType) const
{
    const IExecutionSemanticsProvider *provider = providerForType(componentType);
    if (!provider)
        return {};
    return provider->providedOutputKeys(componentType);
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
    if (componentType == QLatin1String(TypeMod))
        return &m_modProvider;
    if (componentType == QLatin1String(TypeLessThan))
        return &m_lessThanProvider;
    if (componentType == QLatin1String(TypeLessOrEqual))
        return &m_lessOrEqualProvider;
    if (componentType == QLatin1String(TypeEqual))
        return &m_equalProvider;
    if (componentType == QLatin1String(TypeLogicAnd))
        return &m_logicAndProvider;
    return nullptr;
}
