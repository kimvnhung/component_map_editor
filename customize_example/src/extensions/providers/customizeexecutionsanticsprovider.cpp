#include "customizeexecutionsanticsprovider.h"

#include <algorithm>

namespace {

cme::runtime::CompositeGraphDefinition makeSqrtGraphDefinition()
{
        using namespace cme::runtime;

        CompositeGraphDefinition definition;
        definition.componentTypeId = QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSqrtGraph);
        definition.components = {
                { QStringLiteral("entryS"), CompositeExecutionProvider::entryComponentType(), QStringLiteral("S"),
                    QVariantMap{{QStringLiteral("portKey"), QStringLiteral("S")}} },
                { QStringLiteral("sqrt"), QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSqrt), QStringLiteral("sqrt"),
                    QVariantMap{{QStringLiteral("inputRef"), QStringLiteral("sqrt.e1::S")},
                                            {QStringLiteral("outputKey"), QStringLiteral("sqrtS")},
                                            {QStringLiteral("errorKey"), QStringLiteral("error")}} },
                { QStringLiteral("exitS"), CompositeExecutionProvider::exitComponentType(), QStringLiteral("sqrtS"),
                    QVariantMap{{QStringLiteral("portKey"), QStringLiteral("out")}} }
        };
        definition.connections = {
                { QStringLiteral("sqrt.e1"), QStringLiteral("entryS"), QStringLiteral("sqrt"), {} },
                { QStringLiteral("sqrt.e2"), QStringLiteral("sqrt"), QStringLiteral("exitS"), {} }
        };
        definition.inputMappings = {
                { QString(), QStringLiteral("entryS"), QStringLiteral("S"), QStringLiteral("sRef"), QStringLiteral("S") }
        };
        definition.outputMappings = {
                { QStringLiteral("exitS"), QStringLiteral("out"), QStringLiteral("sqrtS"), QStringLiteral("sqrtS") }
        };
        return definition;
}

cme::runtime::CompositeGraphDefinition makeRightTriangleLongestEdgeDefinition()
{
        using namespace cme::runtime;

        CompositeGraphDefinition definition;
        definition.componentTypeId = QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeRightTriangleLongestEdge);
        definition.components = {
                { QStringLiteral("entryA"), CompositeExecutionProvider::entryComponentType(), QStringLiteral("a"),
                    QVariantMap{{QStringLiteral("portKey"), QStringLiteral("a")}} },
                { QStringLiteral("entryB"), CompositeExecutionProvider::entryComponentType(), QStringLiteral("b"),
                    QVariantMap{{QStringLiteral("portKey"), QStringLiteral("b")}} },
                { QStringLiteral("squareA"), QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeMultiply), QStringLiteral("a^2"),
                    QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("edge.a.square::a")},
                                            {QStringLiteral("inputBRef"), QStringLiteral("edge.a.square::a")},
                                            {QStringLiteral("outputKey"), QStringLiteral("a2")},
                                            {QStringLiteral("errorKey"), QStringLiteral("error")}} },
                { QStringLiteral("squareB"), QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeMultiply), QStringLiteral("b^2"),
                    QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("edge.b.square::b")},
                                            {QStringLiteral("inputBRef"), QStringLiteral("edge.b.square::b")},
                                            {QStringLiteral("outputKey"), QStringLiteral("b2")},
                                            {QStringLiteral("errorKey"), QStringLiteral("error")}} },
                { QStringLiteral("sumSquares"), QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeAdd), QStringLiteral("a^2 + b^2"),
                    QVariantMap{{QStringLiteral("inputARef"), QStringLiteral("edge.squareA.sum::a2")},
                                            {QStringLiteral("inputBRef"), QStringLiteral("edge.squareB.sum::b2")},
                                            {QStringLiteral("outputKey"), QStringLiteral("sumSquares")},
                                            {QStringLiteral("errorKey"), QStringLiteral("error")}} },
                { QStringLiteral("sqrtComposite"), QString::fromLatin1(CustomizeExecutionSemanticsProvider::TypeSqrtGraph), QStringLiteral("sqrt"),
                    QVariantMap{{QStringLiteral("sRef"), QStringLiteral("edge.sum.sqrt::sumSquares")}} },
                { QStringLiteral("exitLongest"), CompositeExecutionProvider::exitComponentType(), QStringLiteral("longestEdge"),
                    QVariantMap{{QStringLiteral("portKey"), QStringLiteral("out")}} }
        };
        definition.connections = {
                { QStringLiteral("edge.a.square"), QStringLiteral("entryA"), QStringLiteral("squareA"), {} },
                { QStringLiteral("edge.b.square"), QStringLiteral("entryB"), QStringLiteral("squareB"), {} },
                { QStringLiteral("edge.squareA.sum"), QStringLiteral("squareA"), QStringLiteral("sumSquares"), {} },
                { QStringLiteral("edge.squareB.sum"), QStringLiteral("squareB"), QStringLiteral("sumSquares"), {} },
                { QStringLiteral("edge.sum.sqrt"), QStringLiteral("sumSquares"), QStringLiteral("sqrtComposite"), {} },
                { QStringLiteral("edge.sqrt.exit"), QStringLiteral("sqrtComposite"), QStringLiteral("exitLongest"), {} }
        };
        definition.inputMappings = {
                { QString(), QStringLiteral("entryA"), QStringLiteral("a"), QStringLiteral("sideARef"), QStringLiteral("a") },
                { QString(), QStringLiteral("entryB"), QStringLiteral("b"), QStringLiteral("sideBRef"), QStringLiteral("b") }
        };
        definition.outputMappings = {
                { QStringLiteral("exitLongest"), QStringLiteral("out"), QStringLiteral("longestEdge"), QStringLiteral("sqrtS") }
        };
        return definition;
}

QList<const IExecutionSemanticsProvider *> delegateProviders(
        const CustomizeStartExecutionProvider &startProvider,
        const CustomizeStopExecutionProvider &stopProvider,
        const CustomizeAddExecutionProvider &addProvider,
        const CustomizeSubtractExecutionProvider &subtractProvider,
        const CustomizeMultiplyExecutionProvider &multiplyProvider,
        const CustomizeDivideExecutionProvider &divideProvider,
        const CustomizeSqrtExecutionProvider &sqrtProvider,
        const CustomizeIfElseExecutionProvider &ifElseProvider,
        const CustomizeLoopExecutionProvider &loopProvider,
        const CustomizeErrorHandlerExecutionProvider &errorHandlerProvider,
        const CustomizeLessThanExecutionProvider &lessThanProvider,
        const CustomizeLessOrEqualExecutionProvider &lessOrEqualProvider,
        const CustomizeEqualExecutionProvider &equalProvider,
        const CustomizeLogicAndExecutionProvider &logicAndProvider,
        const CustomizeModExecutionProvider &modProvider)
{
        return {
                &startProvider,
                &stopProvider,
                &addProvider,
                &subtractProvider,
                &multiplyProvider,
                &divideProvider,
                &sqrtProvider,
                &ifElseProvider,
                &loopProvider,
                &errorHandlerProvider,
                &lessThanProvider,
                &lessOrEqualProvider,
                &equalProvider,
                &logicAndProvider,
                &modProvider
        };
}

} // namespace

CustomizeExecutionSemanticsProvider::CustomizeExecutionSemanticsProvider()
{
        m_compositeProvider.setDefinitions({
                makeSqrtGraphDefinition(),
                makeRightTriangleLongestEdgeDefinition()
        });
        m_compositeProvider.setDelegateProviders(delegateProviders(m_startProvider,
                                                                                                                            m_stopProvider,
                                                                                                                            m_addProvider,
                                                                                                                            m_subtractProvider,
                                                                                                                            m_multiplyProvider,
                                                                                                                            m_divideProvider,
                                                                                                                            m_sqrtProvider,
                                                                                                                            m_ifElseProvider,
                                                                                                                            m_loopProvider,
                                                                                                                            m_errorHandlerProvider,
                                                                                                                            m_lessThanProvider,
                                                                                                                            m_lessOrEqualProvider,
                                                                                                                            m_equalProvider,
                                                                                                                            m_logicAndProvider,
                                                                                                                            m_modProvider));
}

namespace {

bool isRefOnlyMathType(const QString &componentType)
{
    return componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeAdd)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeSubtract)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeMultiply)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeDivide)
        || componentType == QLatin1String(CustomizeExecutionSemanticsProvider::TypeSqrt)
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
        QString::fromLatin1(TypeSqrt),
        QString::fromLatin1(TypeErrorHandler),
        QString::fromLatin1(TypeMod),
        QString::fromLatin1(TypeLessThan),
        QString::fromLatin1(TypeLessOrEqual),
        QString::fromLatin1(TypeEqual),
        QString::fromLatin1(TypeLogicAnd),
        QString::fromLatin1(TypeSqrtGraph),
        QString::fromLatin1(TypeRightTriangleLongestEdge)
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
    if (componentType == QLatin1String(TypeSqrt))
        return &m_sqrtProvider;
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
    if (componentType == QLatin1String(TypeSqrtGraph)
        || componentType == QLatin1String(TypeRightTriangleLongestEdge))
        return &m_compositeProvider;
    return nullptr;
}
