#include <QtTest>

#include "services/GraphExecutionSandbox.h"
#include "models/GraphModel.h"
#include "models/ComponentModel.h"
#include "models/ConnectionModel.h"
#include "extensions/contracts/IExecutionSemanticsProvider.h"

class DeterministicExecutionProviderPhase6 : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase6.deterministic");
    }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("process") };
    }

    bool executeComponent(const QString &componentType,
                          const QString &componentId,
                          const QVariantMap &componentSnapshot,
                          const QVariantMap &currentState,
                          QVariantMap *nextState,
                          QVariantMap *trace,
                          QString *error) const override
    {
        Q_UNUSED(componentType)
        Q_UNUSED(componentSnapshot)
        Q_UNUSED(error)

        if (!nextState || !trace)
            return false;

        QVariantMap out = currentState;
        QStringList order = out.value(QStringLiteral("order")).toStringList();
        order.append(componentId);
        out.insert(QStringLiteral("order"), order);
        out.insert(QStringLiteral("lastExecuted"), componentId);
        *nextState = out;

        trace->insert(QStringLiteral("provider"), providerId());
        trace->insert(QStringLiteral("componentId"), componentId);
        return true;
    }
};

class FailingExecutionProviderPhase6 : public IExecutionSemanticsProvider
{
public:
    QString providerId() const override
    {
        return QStringLiteral("phase6.failing");
    }

    QStringList supportedComponentTypes() const override
    {
        return { QStringLiteral("process") };
    }

    bool executeComponent(const QString &, const QString &, const QVariantMap &, const QVariantMap &,
                          QVariantMap *, QVariantMap *, QString *error) const override
    {
        if (error)
            *error = QStringLiteral("Injected failure for testing");
        return false;
    }
};

class tst_Phase6TypedExecutionEvents : public QObject
{
    Q_OBJECT

private:
    static GraphModel *buildLinearGraph(QObject *parent, const QStringList &ids)
    {
        auto *graph = new GraphModel(parent);
        for (const QString &id : ids) {
            auto *c = new ComponentModel(graph);
            c->setId(id);
            c->setType(QStringLiteral("process"));
            c->setTitle(id);
            graph->addComponent(c);
        }

        for (int i = 0; i + 1 < ids.size(); ++i) {
            auto *conn = new ConnectionModel(graph);
            conn->setId(QStringLiteral("e%1").arg(i));
            conn->setSourceId(ids.at(i));
            conn->setTargetId(ids.at(i + 1));
            graph->addConnection(conn);
        }
        return graph;
    }

    static QStringList eventsOf(const QVariantList &timeline)
    {
        QStringList out;
        out.reserve(timeline.size());
        for (const QVariant &v : timeline)
            out.append(v.toMap().value(QStringLiteral("event")).toString());
        return out;
    }

private slots:
    void determinism_eventOrderingAndPayloadsStable()
    {
        QObject owner;
        std::unique_ptr<GraphModel> graph(buildLinearGraph(&owner, {QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")}));

        DeterministicExecutionProviderPhase6 provider;
        GraphExecutionSandbox sandbox;
        sandbox.setGraph(graph.get());
        sandbox.setExecutionSemanticsProviders({ &provider });

        QVERIFY(sandbox.start(QVariantMap{{QStringLiteral("seed"), 42}}));
        sandbox.run();
        const QVariantList firstTimeline = sandbox.timeline();

        QVERIFY(sandbox.start(QVariantMap{{QStringLiteral("seed"), 42}}));
        sandbox.run();
        const QVariantList secondTimeline = sandbox.timeline();

        QCOMPARE(secondTimeline, firstTimeline);

        const QStringList ev = eventsOf(firstTimeline);
        QVERIFY(!ev.isEmpty());
        QCOMPARE(ev.first(), QStringLiteral("simulationStarted"));
        QCOMPARE(ev.last(), QStringLiteral("simulationCompleted"));
        QCOMPARE(ev.count(QStringLiteral("stepExecuted")), 3);

        const QVariantMap startEvent = firstTimeline.first().toMap();
        QVERIFY(startEvent.contains(QStringLiteral("componentCount")));
        QVERIFY(startEvent.contains(QStringLiteral("inputKeys")));
        QVERIFY(startEvent.contains(QStringLiteral("tick")));

        QVariantMap firstStep;
        for (const QVariant &v : firstTimeline) {
            const QVariantMap e = v.toMap();
            if (e.value(QStringLiteral("event")).toString() == QStringLiteral("stepExecuted")) {
                firstStep = e;
                break;
            }
        }
        QVERIFY(!firstStep.isEmpty());
        QVERIFY(firstStep.contains(QStringLiteral("componentId")));
        QVERIFY(firstStep.contains(QStringLiteral("componentType")));
        QVERIFY(firstStep.contains(QStringLiteral("trace")));
    }

    void sequence_startStepBreakpointPauseComplete_stableOrdering()
    {
        QObject owner;
        std::unique_ptr<GraphModel> graph(buildLinearGraph(&owner, {QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C")}));

        DeterministicExecutionProviderPhase6 provider;
        GraphExecutionSandbox sandbox;
        sandbox.setGraph(graph.get());
        sandbox.setExecutionSemanticsProviders({ &provider });
        sandbox.setBreakpoint(QStringLiteral("C"));

        QVERIFY(sandbox.start());
        QVERIFY(sandbox.step());
        QVERIFY(sandbox.run() > 0); // stops on breakpoint C
        QCOMPARE(sandbox.status(), QStringLiteral("paused"));

        QVERIFY(sandbox.step()); // execute C bypassing breakpoint via step
        sandbox.run();
        QCOMPARE(sandbox.status(), QStringLiteral("completed"));

        const QVariantList timeline = sandbox.timeline();
        const QStringList ev = eventsOf(timeline);

        QVERIFY(ev.contains(QStringLiteral("simulationStarted")));
        QVERIFY(ev.contains(QStringLiteral("stepExecuted")));
        QVERIFY(ev.contains(QStringLiteral("breakpointHit")));
        QVERIFY(ev.contains(QStringLiteral("simulationPaused")));
        QVERIFY(ev.contains(QStringLiteral("simulationCompleted")));

        const int idxBreakpoint = ev.indexOf(QStringLiteral("breakpointHit"));
        const int idxPaused = ev.indexOf(QStringLiteral("simulationPaused"));
        QVERIFY(idxBreakpoint >= 0);
        QVERIFY(idxPaused > idxBreakpoint);

        const QVariantMap pausedEvent = timeline.at(idxPaused).toMap();
        QCOMPARE(pausedEvent.value(QStringLiteral("reason")).toString(), QStringLiteral("breakpoint"));
        QVERIFY(pausedEvent.contains(QStringLiteral("componentId")));
    }

    void sequence_errorEventPayloadsStable()
    {
        QObject owner;
        std::unique_ptr<GraphModel> graph(buildLinearGraph(&owner, {QStringLiteral("A")}));

        FailingExecutionProviderPhase6 provider;
        GraphExecutionSandbox sandbox;
        sandbox.setGraph(graph.get());
        sandbox.setExecutionSemanticsProviders({ &provider });

        QVERIFY(sandbox.start());
        const int executed = sandbox.run();
        QCOMPARE(executed, 0);
        QCOMPARE(sandbox.status(), QStringLiteral("error"));

        const QVariantList timeline = sandbox.timeline();
        const QStringList ev = eventsOf(timeline);
        QVERIFY(ev.contains(QStringLiteral("error")));

        QVariantMap errorEvent;
        for (const QVariant &v : timeline) {
            const QVariantMap e = v.toMap();
            if (e.value(QStringLiteral("event")).toString() == QStringLiteral("error")) {
                errorEvent = e;
                break;
            }
        }

        QVERIFY(!errorEvent.isEmpty());
        QVERIFY(errorEvent.contains(QStringLiteral("message")));
        QVERIFY(errorEvent.value(QStringLiteral("message")).toString().contains(QStringLiteral("Injected failure")));
        QVERIFY(errorEvent.contains(QStringLiteral("tick")));
    }
};

QTEST_MAIN(tst_Phase6TypedExecutionEvents)
#include "tst_Phase6TypedExecutionEvents.moc"
