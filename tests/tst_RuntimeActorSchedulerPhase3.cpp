#include <QtTest>

#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <atomic>

#include "component_map_runtime/RuntimeActorScheduler.h"

class tst_RuntimeActorSchedulerPhase3 : public QObject
{
    Q_OBJECT

private slots:
    void fifoAndActorSerialization();
    void independentActorsCanRunInParallel();
    void mailboxBackpressureAndStop();
};

void tst_RuntimeActorSchedulerPhase3::fifoAndActorSerialization()
{
    cme::runtime::RuntimeActorScheduler scheduler(2, 8);
    scheduler.start();
    QMutex mutex;
    QList<int> order;
    int active = 0;
    int maximumActive = 0;
    for (int index = 0; index < 5; ++index)
    {
        QVERIFY(scheduler.submit(QStringLiteral("actor-a"), [&, index] {
            QMutexLocker locker(&mutex);
            ++active;
            maximumActive = qMax(maximumActive, active);
            order.append(index);
            QThread::msleep(2);
            --active;
        }));
    }
    QVERIFY(scheduler.waitForIdle(2000));
    scheduler.stop();
    QCOMPARE(order, QList<int>({0, 1, 2, 3, 4}));
    QCOMPARE(maximumActive, 1);
}

void tst_RuntimeActorSchedulerPhase3::independentActorsCanRunInParallel()
{
    cme::runtime::RuntimeActorScheduler scheduler(2, 2);
    scheduler.start();
    std::atomic<int> active{0};
    std::atomic<int> maximumActive{0};
    for (const QString &actor : {QStringLiteral("actor-a"), QStringLiteral("actor-b")})
    {
        QVERIFY(scheduler.submit(actor, [&] {
            const int current = ++active;
            int observed = maximumActive.load();
            while (current > observed && !maximumActive.compare_exchange_weak(observed, current)) {}
            QThread::msleep(30);
            --active;
        }));
    }
    QVERIFY(scheduler.waitForIdle(2000));
    scheduler.stop();
    QCOMPARE(maximumActive.load(), 2);
}

void tst_RuntimeActorSchedulerPhase3::mailboxBackpressureAndStop()
{
    cme::runtime::RuntimeActorScheduler scheduler(1, 1);
    scheduler.start();
    QMutex mutex;
    QMutexLocker blocker(&mutex);
    QVERIFY(scheduler.submit(QStringLiteral("actor-a"), [&] {
        QMutexLocker locker(&mutex);
    }));
    QVERIFY(!scheduler.submit(QStringLiteral("actor-a"), [] {}));
    QCOMPARE(scheduler.rejectedCount(), qsizetype(1));
    scheduler.stop();
    QVERIFY(!scheduler.isRunning());
    QVERIFY(!scheduler.submit(QStringLiteral("actor-a"), [] {}));
    blocker.unlock();
}

QTEST_MAIN(tst_RuntimeActorSchedulerPhase3)
#include "tst_RuntimeActorSchedulerPhase3.moc"
