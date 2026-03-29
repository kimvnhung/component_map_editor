#include "CompositeExecutionProvider.h"

#include <QElapsedTimer>

#include "models/ComponentModel.h"
#include "models/GraphModel.h"
#include "services/GraphExecutionSandbox.h"

namespace {

QString inputSnapshotKey(const QString &componentId, const QString &portKey)
{
    return QStringLiteral("entry::%1::%2").arg(componentId, portKey);
}

QString componentPortKey(const QVariantMap &componentSnapshot)
{
    return componentSnapshot.value(QStringLiteral("portKey")).toString();
}

QVariantMap mergeIncomingTokens(const cme::execution::IncomingTokens &incomingTokens)
{
    QVariantMap merged;
    QStringList tokenKeys = incomingTokens.keys();
    std::sort(tokenKeys.begin(), tokenKeys.end());
    for (const QString &tokenKey : tokenKeys)
        merged.insert(incomingTokens.value(tokenKey));
    return merged;
}

GraphModel *instantiateGraph(const cme::runtime::CompositeGraphDefinition &definition)
{
    auto *graph = new GraphModel();
    graph->beginBatchUpdate();

    for (const cme::runtime::CompositeComponentSpec &spec : definition.components) {
        auto *component = new ComponentModel(graph);
        component->setId(spec.id);
        component->setType(spec.type);
        component->setTitle(spec.title);

        for (auto it = spec.attributes.constBegin(); it != spec.attributes.constEnd(); ++it)
            component->setDynamicProperty(it.key(), it.value());

        graph->addComponent(component);
    }

    for (const cme::runtime::CompositeConnectionSpec &spec : definition.connections) {
        auto *connection = new ConnectionModel(graph);
        connection->setId(spec.id);
        connection->setSourceId(spec.sourceId);
        connection->setTargetId(spec.targetId);
        connection->setLabel(spec.label);
        connection->setSourceSide(spec.sourceSide);
        connection->setTargetSide(spec.targetSide);
        graph->addConnection(connection);
    }

    graph->endBatchUpdate();
    return graph;
}

} // namespace

namespace cme::runtime {

QString CompositeExecutionProvider::entryComponentType()
{
    return QStringLiteral("runtime/composite.entry");
}

QString CompositeExecutionProvider::exitComponentType()
{
    return QStringLiteral("runtime/composite.exit");
}

CompositeExecutionProvider::CompositeExecutionProvider() = default;

void CompositeExecutionProvider::setDefinitions(const QList<CompositeGraphDefinition> &definitions)
{
    m_definitionsByType.clear();
    for (const CompositeGraphDefinition &definition : definitions)
        m_definitionsByType.insert(definition.componentTypeId, definition);
    validateDefinitions();
}

void CompositeExecutionProvider::setDelegateProviders(const QList<const IExecutionSemanticsProvider *> &providers)
{
    m_delegateProviders = providers;
}

bool CompositeExecutionProvider::isValid() const
{
    return m_valid;
}

QStringList CompositeExecutionProvider::diagnostics() const
{
    return m_diagnostics;
}

QString CompositeExecutionProvider::providerId() const
{
    return QStringLiteral("runtime.compositeExecution");
}

QStringList CompositeExecutionProvider::supportedComponentTypes() const
{
    QStringList out = m_definitionsByType.keys();
    out.append(entryComponentType());
    out.append(exitComponentType());
    out.removeDuplicates();
    std::sort(out.begin(), out.end());
    return out;
}

bool CompositeExecutionProvider::executeComponent(const QString &componentType,
                                                 const QString &componentId,
                                                 const QVariantMap &componentSnapshot,
                                                 const QVariantMap &inputState,
                                                 QVariantMap *outputState,
                                                 QVariantMap *trace,
                                                 QString *error) const
{
    cme::execution::IncomingTokens incomingTokens;
    incomingTokens.insert(QStringLiteral("__legacy_global_state__"), inputState);
    return executeComponentV2(componentType,
                              componentId,
                              componentSnapshot,
                              incomingTokens,
                              outputState,
                              trace,
                              error);
}

bool CompositeExecutionProvider::executeComponentV2(const QString &componentType,
                                                    const QString &componentId,
                                                    const QVariantMap &componentSnapshot,
                                                    const cme::execution::IncomingTokens &incomingTokens,
                                                    cme::execution::ExecutionPayload *outputPayload,
                                                    QVariantMap *trace,
                                                    QString *error) const
{
    if (!m_valid) {
        if (error)
            *error = m_diagnostics.isEmpty()
                ? QStringLiteral("Composite definitions are invalid.")
                : m_diagnostics.first();
        return false;
    }

    if (componentType == entryComponentType()) {
        return executeEntryComponent(componentId, componentSnapshot, incomingTokens, outputPayload, trace, error);
    }

    if (componentType == exitComponentType()) {
        return executeExitComponent(componentId, componentSnapshot, incomingTokens, outputPayload, trace, error);
    }

    const auto it = m_definitionsByType.constFind(componentType);
    if (it == m_definitionsByType.constEnd()) {
        if (error)
            *error = QStringLiteral("No composite definition registered for component type '%1'.")
                .arg(componentType);
        return false;
    }

    return executeCompositeComponent(it.value(), componentId, incomingTokens, outputPayload, trace, error);
}

void CompositeExecutionProvider::validateDefinitions()
{
    QStringList diagnostics;
    const bool ok = validateDefinitionGraph(&diagnostics);
    diagnostics.removeDuplicates();
    std::sort(diagnostics.begin(), diagnostics.end());
    m_diagnostics = diagnostics;
    m_valid = ok && diagnostics.isEmpty();
}

bool CompositeExecutionProvider::validateDefinitionGraph(QStringList *diagnostics) const
{
    bool ok = true;
    QHash<QString, int> visitState;
    QStringList stack;

    std::function<void(const QString &)> dfs = [&](const QString &typeId) {
        visitState[typeId] = 1;
        stack.append(typeId);

        const CompositeGraphDefinition definition = m_definitionsByType.value(typeId);
        QSet<QString> componentIds;
        for (const CompositeComponentSpec &component : definition.components)
            componentIds.insert(component.id);

        for (const CompositeInputPortMapping &mapping : definition.inputMappings) {
            if (!componentIds.contains(mapping.innerEntryComponentId)) {
                diagnostics->append(QStringLiteral(
                    "Composite definition '%1' references missing input entry component '%2'.")
                    .arg(typeId, mapping.innerEntryComponentId));
                ok = false;
            }
        }

        for (const CompositeOutputPortMapping &mapping : definition.outputMappings) {
            if (!componentIds.contains(mapping.innerExitComponentId)) {
                diagnostics->append(QStringLiteral(
                    "Composite definition '%1' references missing output exit component '%2'.")
                    .arg(typeId, mapping.innerExitComponentId));
                ok = false;
            }
        }

        QStringList nestedTypes;
        for (const CompositeComponentSpec &component : definition.components) {
            if (m_definitionsByType.contains(component.type))
                nestedTypes.append(component.type);
        }
        nestedTypes.removeDuplicates();
        std::sort(nestedTypes.begin(), nestedTypes.end());

        for (const QString &nestedType : nestedTypes) {
            const int state = visitState.value(nestedType, 0);
            if (state == 1) {
                const int cycleStart = stack.indexOf(nestedType);
                QStringList cycle = stack.mid(cycleStart);
                cycle.append(nestedType);
                diagnostics->append(QStringLiteral("Composite graph recursion detected: %1")
                                        .arg(cycle.join(QStringLiteral(" -> "))));
                ok = false;
                continue;
            }
            if (state == 0)
                dfs(nestedType);
        }

        stack.removeLast();
        visitState[typeId] = 2;
    };

    QStringList definitionIds = m_definitionsByType.keys();
    std::sort(definitionIds.begin(), definitionIds.end());
    for (const QString &definitionId : definitionIds) {
        if (visitState.value(definitionId, 0) == 0)
            dfs(definitionId);
    }

    return ok;
}

bool CompositeExecutionProvider::executeEntryComponent(const QString &componentId,
                                                       const QVariantMap &componentSnapshot,
                                                       const cme::execution::IncomingTokens &incomingTokens,
                                                       cme::execution::ExecutionPayload *outputPayload,
                                                       QVariantMap *trace,
                                                       QString *error) const
{
    Q_UNUSED(error)

    const QVariantMap graphInput = incomingTokens.value(QStringLiteral("__graph_input__"));
    const QString portKey = componentPortKey(componentSnapshot);
    const QVariant payload = graphInput.value(inputSnapshotKey(componentId, portKey));

    if (outputPayload)
        *outputPayload = payload.toMap();

    if (trace) {
        trace->insert(QStringLiteral("provider"), providerId());
        trace->insert(QStringLiteral("mode"), QStringLiteral("entry"));
        trace->insert(QStringLiteral("portKey"), portKey);
    }

    return true;
}

bool CompositeExecutionProvider::executeExitComponent(const QString &componentId,
                                                      const QVariantMap &componentSnapshot,
                                                      const cme::execution::IncomingTokens &incomingTokens,
                                                      cme::execution::ExecutionPayload *outputPayload,
                                                      QVariantMap *trace,
                                                      QString *error) const
{
    Q_UNUSED(componentId)
    Q_UNUSED(error)

    const QString portKey = componentPortKey(componentSnapshot);
    QVariantMap out;

    QStringList tokenKeys = incomingTokens.keys();
    std::sort(tokenKeys.begin(), tokenKeys.end());

    if (tokenKeys.size() == 1) {
        out.insert(portKey, incomingTokens.value(tokenKeys.first()));
    } else {
        QVariantMap nested;
        for (const QString &tokenKey : tokenKeys)
            nested.insert(tokenKey, incomingTokens.value(tokenKey));
        out.insert(portKey, nested);
    }

    if (outputPayload)
        *outputPayload = out;

    if (trace) {
        trace->insert(QStringLiteral("provider"), providerId());
        trace->insert(QStringLiteral("mode"), QStringLiteral("exit"));
        trace->insert(QStringLiteral("portKey"), portKey);
    }

    return true;
}

bool CompositeExecutionProvider::executeCompositeComponent(const CompositeGraphDefinition &definition,
                                                           const QString &componentId,
                                                           const cme::execution::IncomingTokens &incomingTokens,
                                                           cme::execution::ExecutionPayload *outputPayload,
                                                           QVariantMap *trace,
                                                           QString *error) const
{
    std::unique_ptr<GraphModel> innerGraph(instantiateGraph(definition));

    GraphExecutionSandbox sandbox;
    sandbox.setGraph(innerGraph.get());

    QList<const IExecutionSemanticsProvider *> providers = m_delegateProviders;
    if (!providers.contains(this))
        providers.append(this);
    sandbox.setExecutionSemanticsProviders(providers);

    QVariantMap inputSnapshot;
    for (const CompositeInputPortMapping &mapping : definition.inputMappings) {
        if (!incomingTokens.contains(mapping.outerTokenKey))
            continue;
        inputSnapshot.insert(inputSnapshotKey(mapping.innerEntryComponentId, mapping.innerPortKey),
                             incomingTokens.value(mapping.outerTokenKey));
    }

    if (!sandbox.start(inputSnapshot)) {
        if (error)
            *error = sandbox.lastError();
        return false;
    }

    sandbox.run();
    if (sandbox.status() == QStringLiteral("error")) {
        if (error)
            *error = sandbox.lastError();
        return false;
    }

    QVariantMap out;
    for (const CompositeOutputPortMapping &mapping : definition.outputMappings) {
        const QVariantMap exitState = sandbox.componentState(mapping.innerExitComponentId);
        const QVariantMap exitOutput = exitState.value(QStringLiteral("outputState")).toMap();
        if (!exitOutput.contains(mapping.innerPortKey)) {
            if (error) {
                *error = QStringLiteral("Composite exit '%1' did not produce port '%2'.")
                    .arg(mapping.innerExitComponentId, mapping.innerPortKey);
            }
            return false;
        }

        out.insert(mapping.outerPayloadKey, exitOutput.value(mapping.innerPortKey));
    }

    if (outputPayload)
        *outputPayload = out;

    if (trace) {
        trace->insert(QStringLiteral("provider"), providerId());
        trace->insert(QStringLiteral("mode"), QStringLiteral("composite"));
        trace->insert(QStringLiteral("compositeComponentId"), componentId);
        trace->insert(QStringLiteral("compositeTypeId"), definition.componentTypeId);
        trace->insert(QStringLiteral("innerComponentCount"), definition.components.size());
    }

    return true;
}

} // namespace cme::runtime
