#include <QtTest>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "extensions/runtime/DynamicOptionsRegistry.h"

namespace
{

class FakeExecutionSemanticsProvider : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("test.execution.dynamic-options");
    }

    QStringList supportedComponentTypes() const override
    {
        return {QStringLiteral("typeA"), QStringLiteral("typeC"), QStringLiteral("typeD")};
    }

    QStringList providedOutputKeys(const QString& componentType) const override
    {
        if (componentType == QStringLiteral("typeA"))
            return {QStringLiteral("refA1"), QStringLiteral("refA2")};
        if (componentType == QStringLiteral("typeC"))
            return {QStringLiteral("refA1"), QStringLiteral("refC1"), QStringLiteral("refC2")};
        if (componentType == QStringLiteral("typeD"))
            return {QStringLiteral("refD1")};
        return {};
    }

    bool executeComponent(const QString&, const QString&, const QVariantMap&,
                          const cme::execution::IncomingTokens&,
                          cme::execution::ExecutionPayload*, QVariantMap*, QString*) const override
    {
        return true;
    }
};

void addComponent(GraphModel& graph, const QString& id, const QString& type)
{
    auto* component = new ComponentModel(id, id, 0.0, 0.0, QStringLiteral("#ffffff"), type, &graph);
    graph.addComponent(component);
}

void addConnection(GraphModel& graph, const QString& id, const QString& sourceId, const QString& targetId)
{
    auto* connection = new ConnectionModel(&graph);
    connection->setId(id);
    connection->setSourceId(sourceId);
    connection->setTargetId(targetId);
    graph.addConnection(connection);
}

void prepareRegistry(DynamicOptionsRegistry& registry, GraphModel& graph,
                     ExtensionContractRegistry& contracts, FakeExecutionSemanticsProvider& provider,
                     const QString& targetId)
{
    contracts.registerExecutionSemanticsProvider(&provider);
    registry.rebuildFromRegistry(contracts);
    registry.setGraph(&graph);
    registry.setTargetComponentId(targetId);
}

QVariantMap findOption(const QVariantList& options, const QString& key, const QString& sourceId)
{
    for (const QVariant& value : options)
    {
        const QVariantMap option = value.toMap();
        if (option.value(QStringLiteral("key")).toString() == key &&
            option.value(QStringLiteral("sourceId")).toString() == sourceId)
            return option;
    }
    return {};
}

QVariantList tokenRefOptions(const DynamicOptionsRegistry& registry)
{
    return registry.targetSourceOptions().value(QStringLiteral("tokenRef")).toList();
}

QStringList tokenRefKeys(const DynamicOptionsRegistry& registry)
{
    QStringList keys;
    for (const QVariant& value : tokenRefOptions(registry))
    {
        const QString key = value.toMap().value(QStringLiteral("key")).toString();
        if (!key.isEmpty() && !keys.contains(key))
            keys.append(key);
    }
    return keys;
}

} // namespace

class tst_DynamicOptionsRegistry : public QObject
{
    Q_OBJECT

private slots:
    void collectsOutputKeysFromDirectSources();
    void optionObjectsExposeTextAndValue();
    void duplicateKeysFromDifferentSourcesKeepDistinctValues();
    void targetWithoutIncomingConnectionsHasNoOptions();
    void refreshReactsToConnectionChanges();
    void targetSourceOptionsExposesTokenRefSource();
    void graphComponentOptionsUseTitleAndId();
    void graphComponentOptionsRefreshAfterRemoval();
};

void tst_DynamicOptionsRegistry::collectsOutputKeysFromDirectSources()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("comA"), QStringLiteral("typeA"));
    addComponent(graph, QStringLiteral("comB"), QStringLiteral("typeB"));
    addComponent(graph, QStringLiteral("comC"), QStringLiteral("typeC"));
    addComponent(graph, QStringLiteral("comD"), QStringLiteral("typeD"));
    addConnection(graph, QStringLiteral("edgeA"), QStringLiteral("comA"), QStringLiteral("comB"));
    addConnection(graph, QStringLiteral("edgeC"), QStringLiteral("comC"), QStringLiteral("comB"));
    addConnection(graph, QStringLiteral("edgeD"), QStringLiteral("comD"), QStringLiteral("other"));

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("comB"));

    const QStringList keys = tokenRefKeys(registry);
    QCOMPARE(keys.size(), 4);
    QVERIFY(keys.contains(QStringLiteral("refA1")));
    QVERIFY(keys.contains(QStringLiteral("refA2")));
    QVERIFY(keys.contains(QStringLiteral("refC1")));
    QVERIFY(keys.contains(QStringLiteral("refC2")));
    QVERIFY(!keys.contains(QStringLiteral("refD1")));
}

void tst_DynamicOptionsRegistry::optionObjectsExposeTextAndValue()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("comA"), QStringLiteral("typeA"));
    addComponent(graph, QStringLiteral("comB"), QStringLiteral("typeB"));
    addConnection(graph, QStringLiteral("edgeA"), QStringLiteral("comA"), QStringLiteral("comB"));

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("comB"));

    const QVariantMap option = findOption(tokenRefOptions(registry), QStringLiteral("refA1"), QStringLiteral("comA"));
    QVERIFY(!option.isEmpty());
    QVERIFY(option.contains(QStringLiteral("text")));
    QVERIFY(option.contains(QStringLiteral("value")));
    QCOMPARE(option.value(QStringLiteral("text")).toString(), QStringLiteral("refA1 (comA via edgeA)"));
    QCOMPARE(option.value(QStringLiteral("value")).toString(), QStringLiteral("edgeA::refA1"));
}

void tst_DynamicOptionsRegistry::duplicateKeysFromDifferentSourcesKeepDistinctValues()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("comA"), QStringLiteral("typeA"));
    addComponent(graph, QStringLiteral("comC"), QStringLiteral("typeC"));
    addComponent(graph, QStringLiteral("comB"), QStringLiteral("typeB"));
    addConnection(graph, QStringLiteral("edgeA"), QStringLiteral("comA"), QStringLiteral("comB"));
    addConnection(graph, QStringLiteral("edgeC"), QStringLiteral("comC"), QStringLiteral("comB"));

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("comB"));

    QVERIFY(tokenRefKeys(registry).contains(QStringLiteral("refA1")));
    QCOMPARE(tokenRefKeys(registry).count(QStringLiteral("refA1")), 1);

    const QVariantMap optionA = findOption(tokenRefOptions(registry), QStringLiteral("refA1"), QStringLiteral("comA"));
    const QVariantMap optionC = findOption(tokenRefOptions(registry), QStringLiteral("refA1"), QStringLiteral("comC"));
    QVERIFY(!optionA.isEmpty());
    QVERIFY(!optionC.isEmpty());
    QVERIFY(optionA.value(QStringLiteral("value")).toString() != optionC.value(QStringLiteral("value")).toString());
}

void tst_DynamicOptionsRegistry::targetWithoutIncomingConnectionsHasNoOptions()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("comB"), QStringLiteral("typeB"));

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("comB"));

    QVERIFY(tokenRefKeys(registry).isEmpty());
    QVERIFY(tokenRefOptions(registry).isEmpty());
}

void tst_DynamicOptionsRegistry::refreshReactsToConnectionChanges()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("comA"), QStringLiteral("typeA"));
    addComponent(graph, QStringLiteral("comB"), QStringLiteral("typeB"));

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("comB"));
    QVERIFY(tokenRefKeys(registry).isEmpty());

    addConnection(graph, QStringLiteral("edgeA"), QStringLiteral("comA"), QStringLiteral("comB"));
    QVERIFY(tokenRefKeys(registry).contains(QStringLiteral("refA1")));

    QVERIFY(graph.removeConnection(QStringLiteral("edgeA")));
    QVERIFY(tokenRefKeys(registry).isEmpty());
    QVERIFY(tokenRefOptions(registry).isEmpty());
}

void tst_DynamicOptionsRegistry::targetSourceOptionsExposesTokenRefSource()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("comA"), QStringLiteral("typeA"));
    addComponent(graph, QStringLiteral("comB"), QStringLiteral("typeB"));
    addConnection(graph, QStringLiteral("edgeA"), QStringLiteral("comA"), QStringLiteral("comB"));

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("comB"));

    const QVariantMap sources = registry.targetSourceOptions();
    QVERIFY(sources.contains(QStringLiteral("tokenRef")));
    QVERIFY(!sources.value(QStringLiteral("tokenRef")).toList().isEmpty());
}

void tst_DynamicOptionsRegistry::graphComponentOptionsUseTitleAndId()
{
    GraphModel graph;
    auto *component = new ComponentModel(QStringLiteral("component-a"), QStringLiteral("Display A"),
                                         0.0, 0.0, QStringLiteral("#ffffff"), QStringLiteral("typeA"), &graph);
    graph.addComponent(component);

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("graph"));

    const QVariantList options = registry.targetSourceOptions().value(QStringLiteral("components")).toList();
    QCOMPARE(options.size(), 1);
    const QVariantMap option = options.first().toMap();
    QCOMPARE(option.value(QStringLiteral("text")).toString(), QStringLiteral("Display A"));
    QCOMPARE(option.value(QStringLiteral("value")).toString(), QStringLiteral("component-a"));
}

void tst_DynamicOptionsRegistry::graphComponentOptionsRefreshAfterRemoval()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("component-a"), QStringLiteral("typeA"));
    addComponent(graph, QStringLiteral("component-b"), QStringLiteral("typeB"));

    ExtensionContractRegistry contracts(ExtensionApiVersion{1, 0, 0});
    FakeExecutionSemanticsProvider provider;
    DynamicOptionsRegistry registry;
    prepareRegistry(registry, graph, contracts, provider, QStringLiteral("graph"));
    QVERIFY(graph.removeComponent(QStringLiteral("component-a")));

    const QVariantList options = registry.targetSourceOptions().value(QStringLiteral("components")).toList();
    QCOMPARE(options.size(), 1);
    QCOMPARE(options.first().toMap().value(QStringLiteral("value")).toString(), QStringLiteral("component-b"));
}

QTEST_MAIN(tst_DynamicOptionsRegistry)
#include "tst_DynamicOptionsRegistry.moc"
