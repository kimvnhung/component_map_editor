#include <QtTest/QtTest>

#include <QElapsedTimer>

#include "GraphModel.h"

class GraphModelIndexingTests : public QObject
{
    Q_OBJECT

private slots:
    void componentLookupTracksIdChanges();
    void connectionLookupTracksIdChanges();
    void removeUsesFirstMatchWithDuplicateIds();
    void clearResetsIndexesAndAllowsReuse();
    void lookupBenchmarkLargeGraph();
    void lookupBeatsLinearBaseline();
};

namespace {

ComponentModel *linearComponentLookup(const GraphModel &graph, const QString &id)
{
    const auto &components = graph.componentList();
    for (ComponentModel *component : components) {
        if (component && component->id() == id)
            return component;
    }
    return nullptr;
}

} // namespace

void GraphModelIndexingTests::componentLookupTracksIdChanges()
{
    GraphModel graph;

    auto *a = new ComponentModel(QStringLiteral("A"), QStringLiteral("A"), 0.0, 0.0);
    auto *b = new ComponentModel(QStringLiteral("B"), QStringLiteral("B"), 1.0, 1.0);
    graph.addComponent(a);
    graph.addComponent(b);

    QCOMPARE(graph.componentById(QStringLiteral("A")), a);
    QCOMPARE(graph.componentById(QStringLiteral("B")), b);

    a->setId(QStringLiteral("A2"));
    QCOMPARE(graph.componentById(QStringLiteral("A")), static_cast<ComponentModel *>(nullptr));
    QCOMPARE(graph.componentById(QStringLiteral("A2")), a);

    b->setId(QStringLiteral("A2"));
    // First-match behavior from source list must remain stable.
    QCOMPARE(graph.componentById(QStringLiteral("A2")), a);
}

void GraphModelIndexingTests::connectionLookupTracksIdChanges()
{
    GraphModel graph;

    auto *c1 = new ComponentModel(QStringLiteral("C1"), QStringLiteral("C1"), 0.0, 0.0);
    auto *c2 = new ComponentModel(QStringLiteral("C2"), QStringLiteral("C2"), 1.0, 1.0);
    graph.addComponent(c1);
    graph.addComponent(c2);

    auto *e1 = new ConnectionModel(QStringLiteral("E1"), QStringLiteral("C1"), QStringLiteral("C2"));
    graph.addConnection(e1);

    QCOMPARE(graph.connectionById(QStringLiteral("E1")), e1);

    e1->setId(QStringLiteral("E2"));
    QCOMPARE(graph.connectionById(QStringLiteral("E1")), static_cast<ConnectionModel *>(nullptr));
    QCOMPARE(graph.connectionById(QStringLiteral("E2")), e1);
}

void GraphModelIndexingTests::removeUsesFirstMatchWithDuplicateIds()
{
    GraphModel graph;
    graph.beginBatchUpdate();

    auto *first = new ComponentModel(QStringLiteral("dup"), QStringLiteral("first"), 0.0, 0.0);
    auto *second = new ComponentModel(QStringLiteral("dup"), QStringLiteral("second"), 2.0, 2.0);
    graph.addComponent(first);
    graph.addComponent(second);

    auto *eFirst = new ConnectionModel(QStringLiteral("dupE"), QStringLiteral("dup"), QStringLiteral("dup"));
    auto *eSecond = new ConnectionModel(QStringLiteral("dupE"), QStringLiteral("dup"), QStringLiteral("dup"));
    graph.addConnection(eFirst);
    graph.addConnection(eSecond);
    graph.endBatchUpdate();

    QCOMPARE(graph.componentById(QStringLiteral("dup")), first);
    QVERIFY(graph.removeComponent(QStringLiteral("dup")));
    QCOMPARE(graph.componentById(QStringLiteral("dup")), second);

    QCOMPARE(graph.connectionById(QStringLiteral("dupE")), eFirst);
    QVERIFY(graph.removeConnection(QStringLiteral("dupE")));
    QCOMPARE(graph.connectionById(QStringLiteral("dupE")), eSecond);
}

void GraphModelIndexingTests::clearResetsIndexesAndAllowsReuse()
{
    GraphModel graph;

    auto *component = new ComponentModel(QStringLiteral("X"), QStringLiteral("X"), 0.0, 0.0);
    auto *target = new ComponentModel(QStringLiteral("Y"), QStringLiteral("Y"), 1.0, 1.0);
    auto *connection = new ConnectionModel(QStringLiteral("L"), QStringLiteral("X"), QStringLiteral("Y"));
    graph.addComponent(component);
    graph.addComponent(target);
    graph.addConnection(connection);

    graph.clear();

    QCOMPARE(graph.componentById(QStringLiteral("X")), static_cast<ComponentModel *>(nullptr));
    QCOMPARE(graph.connectionById(QStringLiteral("L")), static_cast<ConnectionModel *>(nullptr));

    auto *newComponent = new ComponentModel(QStringLiteral("X"), QStringLiteral("new"), 5.0, 5.0);
    graph.addComponent(newComponent);
    QCOMPARE(graph.componentById(QStringLiteral("X")), newComponent);
}

void GraphModelIndexingTests::lookupBenchmarkLargeGraph()
{
    GraphModel graph;
    graph.beginBatchUpdate();
    for (int i = 0; i < 20000; ++i) {
        auto *c = new ComponentModel(QStringLiteral("n%1").arg(i),
                                     QStringLiteral("N%1").arg(i),
                                     static_cast<qreal>(i),
                                     static_cast<qreal>(i));
        graph.addComponent(c);
    }
    graph.endBatchUpdate();

    QElapsedTimer timer;
    timer.start();

    int hits = 0;
    for (int pass = 0; pass < 5; ++pass) {
        for (int i = 0; i < 20000; ++i)
            hits += graph.componentById(QStringLiteral("n%1").arg(i)) ? 1 : 0;
    }

    const qint64 elapsedMs = timer.elapsed();
    QCOMPARE(hits, 100000);
    // Profiling smoke-check: indexed lookups should remain comfortably sub-second.
    QVERIFY2(elapsedMs < 1000, "Indexed lookup profiling exceeded expected budget");
}

void GraphModelIndexingTests::lookupBeatsLinearBaseline()
{
    GraphModel graph;
    graph.beginBatchUpdate();
    for (int i = 0; i < 20000; ++i) {
        auto *c = new ComponentModel(QStringLiteral("cmp%1").arg(i),
                                     QStringLiteral("Cmp%1").arg(i),
                                     static_cast<qreal>(i),
                                     static_cast<qreal>(i));
        graph.addComponent(c);
    }
    graph.endBatchUpdate();

    QElapsedTimer indexedTimer;
    indexedTimer.start();
    int indexedHits = 0;
    for (int pass = 0; pass < 3; ++pass) {
        for (int i = 0; i < 20000; ++i)
            indexedHits += graph.componentById(QStringLiteral("cmp%1").arg(i)) ? 1 : 0;
    }
    const qint64 indexedMs = indexedTimer.elapsed();

    QElapsedTimer linearTimer;
    linearTimer.start();
    int linearHits = 0;
    for (int pass = 0; pass < 3; ++pass) {
        for (int i = 0; i < 20000; ++i)
            linearHits += linearComponentLookup(graph, QStringLiteral("cmp%1").arg(i)) ? 1 : 0;
    }
    const qint64 linearMs = linearTimer.elapsed();

    QCOMPARE(indexedHits, 60000);
    QCOMPARE(linearHits, 60000);

    // Performance gate: indexed lookup should be substantially faster than linear scan.
    QVERIFY2(indexedMs * 5 <= linearMs,
             "Indexed lookup did not beat linear baseline by required margin");
}

QTEST_MAIN(GraphModelIndexingTests)
#include "tst_GraphModelIndexing.moc"
