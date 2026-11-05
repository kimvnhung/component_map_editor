#include <QtTest>

#include "models/TimelineModel.h"

class TimelineDiscoveryLifecycleTest : public QObject
{
    Q_OBJECT

private slots:
    void appendsLifecycleRowsWithStableReference();
    void ignoresMalformedLifecycleRows();
};

void TimelineDiscoveryLifecycleTest::appendsLifecycleRowsWithStableReference()
{
    TimelineModel model;
    model.appendDiscoveryLifecycleEvents({QVariantMap{
        {QStringLiteral("eventId"), QStringLiteral("event-1")},
        {QStringLiteral("kind"), QStringLiteral("unavailable")},
        {QStringLiteral("status"), QStringLiteral("unavailable")},
        {QStringLiteral("componentIdentity"), QStringLiteral("runtime.transform")},
        {QStringLiteral("componentVersion"), QStringLiteral("2.1.0")},
        {QStringLiteral("occurredAtUnixMs"), qlonglong(1234)},
    }});

    QCOMPARE(model.rowCount(), 1);
    const QVariantMap row = model.toVariantList().first().toMap();
    QCOMPARE(row.value(QStringLiteral("event")).toString(),
             QStringLiteral("discovery.unavailable"));
    QCOMPARE(row.value(QStringLiteral("eventId")).toString(), QStringLiteral("event-1"));
    QCOMPARE(row.value(QStringLiteral("componentIdentity")).toString(),
             QStringLiteral("runtime.transform"));
    QCOMPARE(row.value(QStringLiteral("componentVersion")).toString(), QStringLiteral("2.1.0"));
    QCOMPARE(row.value(QStringLiteral("source")).toString(),
             QStringLiteral("runtime.discovery"));
}

void TimelineDiscoveryLifecycleTest::ignoresMalformedLifecycleRows()
{
    TimelineModel model;
    model.appendDiscoveryLifecycleEvents({
        QVariantMap{{QStringLiteral("kind"), QStringLiteral("registered")}},
        QVariantMap{{QStringLiteral("eventId"), QStringLiteral("event-2")}},
    });
    QCOMPARE(model.rowCount(), 0);
}

QTEST_MAIN(TimelineDiscoveryLifecycleTest)
#include "tst_TimelineDiscoveryLifecycle.moc"
