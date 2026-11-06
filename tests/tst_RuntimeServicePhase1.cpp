#include <QtTest>

#include "component_map_runtime/RuntimeHandlerRegistry.h"
#include "component_map_runtime/RuntimeService.h"

namespace
{

class FakeHandler : public cme::runtime::RuntimeHandler
{
public:
    explicit FakeHandler(QString type)
        : m_type(std::move(type))
    {
    }

    QString componentType() const override
    {
        return m_type;
    }

protected:
    bool validate(const cme::runtime::RuntimeEvent &event,
                  cme::runtime::RuntimeEvent *output,
                  cme::runtime::RuntimeFailure *failure) const
    {
        if (!event.eventId.isEmpty() && output && failure)
        {
            return true;
        }

        if (failure)
        {
            failure->code = QStringLiteral("invalid_event");
            failure->message = QStringLiteral("Event and output are required.");
        }
        return false;
    }

    QString m_type;
};

class ProducerHandler final : public FakeHandler
{
public:
    ProducerHandler()
        : FakeHandler(QStringLiteral("producer"))
    {
    }

    bool process(const cme::runtime::RuntimeEvent &event,
                 QByteArray &state,
                 cme::runtime::RuntimeEvent *output,
                 cme::runtime::RuntimeFailure *failure) override
    {
        if (!validate(event, output, failure))
        {
            return false;
        }
        state.append('p');
        *output = event;
        output->sourceComponentId = QStringLiteral("producer-01");
        output->targetComponentId = QStringLiteral("processor-01");
        failure->code.clear();
        failure->message.clear();
        return true;
    }
};

class ProcessorHandler final : public FakeHandler
{
public:
    ProcessorHandler()
        : FakeHandler(QStringLiteral("processor"))
    {
    }

    bool process(const cme::runtime::RuntimeEvent &event,
                 QByteArray &state,
                 cme::runtime::RuntimeEvent *output,
                 cme::runtime::RuntimeFailure *failure) override
    {
        if (!validate(event, output, failure))
        {
            return false;
        }
        state.append('x');
        *output = event;
        output->sourceComponentId = QStringLiteral("processor-01");
        output->targetComponentId = QStringLiteral("sink-01");
        failure->code.clear();
        failure->message.clear();
        return true;
    }
};

class SinkHandler final : public FakeHandler
{
public:
    SinkHandler()
        : FakeHandler(QStringLiteral("sink"))
    {
    }

    bool process(const cme::runtime::RuntimeEvent &event,
                 QByteArray &state,
                 cme::runtime::RuntimeEvent *output,
                 cme::runtime::RuntimeFailure *failure) override
    {
        if (!validate(event, output, failure))
        {
            return false;
        }
        state.append('s');
        *output = event;
        output->sourceComponentId = QStringLiteral("sink-01");
        output->targetComponentId.clear();
        failure->code.clear();
        failure->message.clear();
        return true;
    }
};

struct GraphFixture
{
    struct Route
    {
        QString source;
        QString target;
    };

    QVector<cme::runtime::RuntimeComponentIdentity> components{
        {QStringLiteral("producer-01"), QStringLiteral("producer")},
        {QStringLiteral("processor-01"), QStringLiteral("processor")},
        {QStringLiteral("sink-01"), QStringLiteral("sink")}};
    QVector<Route> routes{{QStringLiteral("producer-01"), QStringLiteral("processor-01")},
                          {QStringLiteral("processor-01"), QStringLiteral("sink-01")}};
};

void registerFakeHandlers(cme::runtime::RuntimeHandlerRegistry &registry)
{
    QVERIFY(registry.registerFactory(
        QStringLiteral("producer"), [] { return std::make_unique<ProducerHandler>(); }));
    QVERIFY(registry.registerFactory(
        QStringLiteral("processor"), [] { return std::make_unique<ProcessorHandler>(); }));
    QVERIFY(registry.registerFactory(QStringLiteral("sink"), [] { return std::make_unique<SinkHandler>(); }));
}

} // namespace

class tst_RuntimeServicePhase1 : public QObject
{
    Q_OBJECT

private slots:
    void invalidDescriptorIsRejected();
    void validDescriptorHasDeterministicLifecycle();
    void statusSnapshotRoundTripsDescriptor();
    void handlerRegistryCreatesRegisteredHandler();
    void producerProcessorSinkFixtureRunsThroughRegistry();
    void componentInstancesKeepIdentityAndStateSeparate();

};

void tst_RuntimeServicePhase1::invalidDescriptorIsRejected()
{
    cme::runtime::RuntimeService service;
    QString error;

    QVERIFY(!service.validateConfiguration(&error));
    QCOMPARE(error, QStringLiteral("Runtime ID must not be empty."));
    QVERIFY(!service.start(&error));
    QCOMPARE(service.status(), cme::runtime::v1::RUNTIME_STATUS_FAILED);
    QCOMPARE(service.lastError(), error);
}

void tst_RuntimeServicePhase1::validDescriptorHasDeterministicLifecycle()
{
    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id("test-runtime");
    descriptor.set_protocol_version("1.0");
    descriptor.set_graph_version("graph-v1");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);

    cme::runtime::RuntimeService service(descriptor);
    QString error;
    QVERIFY(service.start(&error));
    QVERIFY(error.isEmpty());
    QCOMPARE(service.status(), cme::runtime::v1::RUNTIME_STATUS_RUNNING);
    QVERIFY(service.start(&error));

    QVERIFY(service.stop(&error));
    QVERIFY(error.isEmpty());
    QCOMPARE(service.status(), cme::runtime::v1::RUNTIME_STATUS_STOPPED);
    QVERIFY(service.stop(&error));
}

void tst_RuntimeServicePhase1::statusSnapshotRoundTripsDescriptor()
{
    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id("snapshot-runtime");
    descriptor.set_protocol_version("1.0");
    descriptor.set_graph_version("graph-v2");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_EMBEDDED);

    cme::runtime::RuntimeService service(descriptor);
    const cme::runtime::v1::RuntimeStatusSnapshot snapshot = service.statusSnapshot();

    QCOMPARE(snapshot.runtime_descriptor().runtime_id(), std::string("snapshot-runtime"));
    QCOMPARE(snapshot.runtime_descriptor().protocol_version(), std::string("1.0"));
    QCOMPARE(snapshot.runtime_descriptor().graph_version(), std::string("graph-v2"));
    QCOMPARE(snapshot.runtime_descriptor().mode(), cme::runtime::v1::RUNTIME_MODE_EMBEDDED);
    QCOMPARE(snapshot.status(), cme::runtime::v1::RUNTIME_STATUS_STOPPED);
}

void tst_RuntimeServicePhase1::handlerRegistryCreatesRegisteredHandler()
{
    cme::runtime::RuntimeHandlerRegistry registry;
    QVERIFY(registry.registerFactory(
        QStringLiteral("processor"),
        [] { return std::make_unique<ProcessorHandler>(); }));
    QVERIFY(registry.contains(QStringLiteral("processor")));
    QVERIFY(!registry.registerFactory(
        QStringLiteral("processor"),
        [] { return std::make_unique<ProcessorHandler>(); }));

    auto handler = registry.create(QStringLiteral("processor"));
    QVERIFY(handler != nullptr);
    QCOMPARE(handler->componentType(), QStringLiteral("processor"));
    QVERIFY(registry.create(QStringLiteral("missing")) == nullptr);
}

void tst_RuntimeServicePhase1::producerProcessorSinkFixtureRunsThroughRegistry()
{
    const GraphFixture fixture;
    QCOMPARE(fixture.components.size(), 3);
    QCOMPARE(fixture.routes.size(), 2);
    for (const auto &component : fixture.components)
    {
        QVERIFY(component.isValid());
    }

    cme::runtime::RuntimeHandlerRegistry registry;
    registerFakeHandlers(registry);

    QHash<QString, QByteArray> state;
    cme::runtime::RuntimeEvent event;
    event.eventId = QStringLiteral("fixture-event-1");
    event.messageType = QStringLiteral("fixture.input");
    event.sourceComponentId = QStringLiteral("external");
    event.targetComponentId = QStringLiteral("producer-01");

    for (const auto &component : fixture.components)
    {
        auto handler = registry.create(component.componentType);
        QVERIFY(handler != nullptr);
        cme::runtime::RuntimeEvent output;
        cme::runtime::RuntimeFailure failure;
        QVERIFY2(handler->process(event, state[component.instanceId], &output, &failure),
                 qPrintable(failure.message));
        QVERIFY(failure.code.isEmpty());
        QCOMPARE(output.sourceComponentId, component.instanceId);
        if (component.instanceId != QStringLiteral("sink-01"))
        {
            QCOMPARE(output.targetComponentId, fixture.routes[component.instanceId == QStringLiteral("producer-01") ? 0 : 1].target);
        }
        event = output;
    }

    QCOMPARE(state.value(QStringLiteral("producer-01")), QByteArrayLiteral("p"));
    QCOMPARE(state.value(QStringLiteral("processor-01")), QByteArrayLiteral("x"));
    QCOMPARE(state.value(QStringLiteral("sink-01")), QByteArrayLiteral("s"));
}

void tst_RuntimeServicePhase1::componentInstancesKeepIdentityAndStateSeparate()
{
    cme::runtime::RuntimeComponentIdentity producerA{
        QStringLiteral("producer-a"), QStringLiteral("producer")};
    cme::runtime::RuntimeComponentIdentity producerB{
        QStringLiteral("producer-b"), QStringLiteral("producer")};
    QVERIFY(producerA.isValid());
    QVERIFY(producerB.isValid());
    QVERIFY(producerA.instanceId != producerB.instanceId);
    QCOMPARE(producerA.componentType, producerB.componentType);

    ProcessorHandler handlerA;
    ProcessorHandler handlerB;
    QByteArray stateA;
    QByteArray stateB;
    cme::runtime::RuntimeEvent event;
    event.eventId = QStringLiteral("event-1");
    event.messageType = QStringLiteral("input");

    cme::runtime::RuntimeEvent output;
    cme::runtime::RuntimeFailure failure;
    QVERIFY(handlerA.process(event, stateA, &output, &failure));
    QVERIFY(stateA == QByteArrayLiteral("x"));
    QVERIFY(stateB.isEmpty());
    QVERIFY(handlerB.process(event, stateB, &output, &failure));
    QVERIFY(stateB == QByteArrayLiteral("x"));
    QVERIFY(stateA == stateB);
}

QTEST_MAIN(tst_RuntimeServicePhase1)
#include "tst_RuntimeServicePhase1.moc"
