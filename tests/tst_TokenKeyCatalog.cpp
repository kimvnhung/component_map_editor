#include <QtTest>

#include "models/ConnectionModel.h"
#include "models/GraphModel.h"
#include "services/TokenKeyCatalog.h"

namespace {

ConnectionModel *makeConnection(GraphModel &graph,
                                const QString &id,
                                const QString &sourceId,
                                const QString &targetId,
                                const QString &tokenKey)
{
    auto *connection = new ConnectionModel(&graph);
    connection->setId(id);
    connection->setSourceId(sourceId);
    connection->setTargetId(targetId);
    connection->setTokenKey(tokenKey);
    graph.addConnection(connection);
    return connection;
}

} // namespace

class tst_TokenKeyCatalog : public QObject
{
    Q_OBJECT

private slots:
    void runtimeOptionExtraction_prefersIncomingPayloadKeys();
    void fallbackToConnectionTokenKey_whenRuntimePayloadUnavailable();
};

void tst_TokenKeyCatalog::runtimeOptionExtraction_prefersIncomingPayloadKeys()
{
    GraphModel graph;
    makeConnection(graph,
                   QStringLiteral("edge.rdc.crc"),
                   QStringLiteral("RDC"),
                   QStringLiteral("CRC"),
                   QStringLiteral("tok.rdc.crc"));

    TokenKeyCatalog catalog;
    catalog.setGraph(&graph);
    catalog.setTargetComponentId(QStringLiteral("CRC"));

    QVariantMap executionState;
    executionState.insert(
        QStringLiteral("incomingTokenPayloads"),
        QVariantMap{{QStringLiteral("edge.rdc.crc"),
                     QVariantMap{{QStringLiteral("rd_key1"), 10},
                                 {QStringLiteral("rd_key2"), 20}}}});
    catalog.setExecutionStateSnapshot(executionState);

    const QStringList keys = catalog.tokenKeys();
    QVERIFY(keys.contains(QStringLiteral("rd_key1")));
    QVERIFY(keys.contains(QStringLiteral("rd_key2")));

    const QVariantList options = catalog.tokenKeyOptions();
    bool hasRdKey1 = false;
    bool hasRdKey2 = false;
    for (const QVariant &value : options) {
        const QVariantMap row = value.toMap();
        if (row.value(QStringLiteral("key")).toString() == QStringLiteral("rd_key1")
            && row.value(QStringLiteral("value")).toString() == QStringLiteral("edge.rdc.crc::rd_key1")) {
            hasRdKey1 = true;
        }
        if (row.value(QStringLiteral("key")).toString() == QStringLiteral("rd_key2")
            && row.value(QStringLiteral("value")).toString() == QStringLiteral("edge.rdc.crc::rd_key2")) {
            hasRdKey2 = true;
        }
    }

    QVERIFY(hasRdKey1);
    QVERIFY(hasRdKey2);
}

void tst_TokenKeyCatalog::fallbackToConnectionTokenKey_whenRuntimePayloadUnavailable()
{
    GraphModel graph;
    makeConnection(graph,
                   QStringLiteral("edge.rdc.crc"),
                   QStringLiteral("RDC"),
                   QStringLiteral("CRC"),
                   QStringLiteral("tok.rdc.crc"));

    TokenKeyCatalog catalog;
    catalog.setGraph(&graph);
    catalog.setTargetComponentId(QStringLiteral("CRC"));
    catalog.setExecutionStateSnapshot(QVariantMap{});

    const QStringList keys = catalog.tokenKeys();
    QCOMPARE(keys, QStringList{QStringLiteral("tok.rdc.crc")});

    const QVariantList options = catalog.tokenKeyOptions();
    QVERIFY(!options.isEmpty());
    const QVariantMap first = options.first().toMap();
    QCOMPARE(first.value(QStringLiteral("value")).toString(), QStringLiteral("tok.rdc.crc"));
}

QTEST_MAIN(tst_TokenKeyCatalog)
#include "tst_TokenKeyCatalog.moc"
