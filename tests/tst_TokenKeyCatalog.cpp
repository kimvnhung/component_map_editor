#include <QtTest>

#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/TokenKeyCatalog.h"

namespace {

void addComponent(GraphModel &graph, const QString &id, const QString &type)
{
    auto *component = new ComponentModel(id, id, 0.0, 0.0, QStringLiteral("#ffffff"), type, &graph);
    graph.addComponent(component);
}

void addConnection(GraphModel &graph, const QString &id,
                   const QString &sourceId, const QString &targetId)
{
    auto *connection = new ConnectionModel(&graph);
    connection->setId(id);
    connection->setSourceId(sourceId);
    connection->setTargetId(targetId);
    graph.addConnection(connection);
}

} // namespace

class tst_TokenKeyCatalog : public QObject
{
    Q_OBJECT

private slots:
    void showsDeclaredKeysForIncomingSourceType();
    void emptyWhenNoHintsRegistered();
    void multipleSources_unionsKeys();
    void updatesWhenHintsChange();
};

void tst_TokenKeyCatalog::showsDeclaredKeysForIncomingSourceType()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("S1"), QStringLiteral("start"));
    addComponent(graph, QStringLiteral("P1"), QStringLiteral("process"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("S1"), QStringLiteral("P1"));

    TokenKeyCatalog catalog;
    catalog.setGraph(&graph);
    catalog.setTargetComponentId(QStringLiteral("P1"));
    catalog.setProviderOutputKeyHints(QVariantMap{
        { QStringLiteral("start"),
          QStringList{ QStringLiteral("workingNumber"),
                       QStringLiteral("started"),
                       QStringLiteral("inputNumber") } }
    });

    const QStringList keys = catalog.tokenKeys();
    QVERIFY(keys.contains(QStringLiteral("workingNumber")));
    QVERIFY(keys.contains(QStringLiteral("started")));
    QVERIFY(keys.contains(QStringLiteral("inputNumber")));

    // Options carry sourceId for display context
    bool foundWithSourceId = false;
    for (const QVariant &v : catalog.tokenKeyOptions()) {
        const QVariantMap row = v.toMap();
        if (row.value(QStringLiteral("key")).toString() == QStringLiteral("workingNumber")
            && row.value(QStringLiteral("sourceId")).toString() == QStringLiteral("S1")) {
            foundWithSourceId = true;
            break;
        }
    }
    QVERIFY(foundWithSourceId);
}

void tst_TokenKeyCatalog::emptyWhenNoHintsRegistered()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("S1"), QStringLiteral("start"));
    addComponent(graph, QStringLiteral("P1"), QStringLiteral("process"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("S1"), QStringLiteral("P1"));

    TokenKeyCatalog catalog;
    catalog.setGraph(&graph);
    catalog.setTargetComponentId(QStringLiteral("P1"));
    // No providerOutputKeyHints set — catalog should be empty

    QVERIFY(catalog.tokenKeys().isEmpty());
    QVERIFY(catalog.tokenKeyOptions().isEmpty());
}

void tst_TokenKeyCatalog::multipleSources_unionsKeys()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("S1"), QStringLiteral("start"));
    addComponent(graph, QStringLiteral("P2"), QStringLiteral("process"));
    addComponent(graph, QStringLiteral("T1"), QStringLiteral("stop"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("S1"), QStringLiteral("T1"));
    addConnection(graph, QStringLiteral("e2"), QStringLiteral("P2"), QStringLiteral("T1"));

    TokenKeyCatalog catalog;
    catalog.setGraph(&graph);
    catalog.setTargetComponentId(QStringLiteral("T1"));
    catalog.setProviderOutputKeyHints(QVariantMap{
        { QStringLiteral("start"),
          QStringList{ QStringLiteral("workingNumber"), QStringLiteral("started") } },
        { QStringLiteral("process"),
          QStringList{ QStringLiteral("workingNumber"), QStringLiteral("lastProcessAddValue") } }
    });

    const QStringList keys = catalog.tokenKeys();
    // workingNumber appears in both — deduplicated
    QCOMPARE(keys.count(QStringLiteral("workingNumber")), 1);
    QVERIFY(keys.contains(QStringLiteral("started")));
    QVERIFY(keys.contains(QStringLiteral("lastProcessAddValue")));
}

void tst_TokenKeyCatalog::updatesWhenHintsChange()
{
    GraphModel graph;
    addComponent(graph, QStringLiteral("S1"), QStringLiteral("start"));
    addComponent(graph, QStringLiteral("P1"), QStringLiteral("process"));
    addConnection(graph, QStringLiteral("e1"), QStringLiteral("S1"), QStringLiteral("P1"));

    TokenKeyCatalog catalog;
    catalog.setGraph(&graph);
    catalog.setTargetComponentId(QStringLiteral("P1"));

    QVERIFY(catalog.tokenKeys().isEmpty());

    catalog.setProviderOutputKeyHints(QVariantMap{
        { QStringLiteral("start"),
          QStringList{ QStringLiteral("workingNumber") } }
    });

    QVERIFY(catalog.tokenKeys().contains(QStringLiteral("workingNumber")));
}

QTEST_MAIN(tst_TokenKeyCatalog)
#include "tst_TokenKeyCatalog.moc"

