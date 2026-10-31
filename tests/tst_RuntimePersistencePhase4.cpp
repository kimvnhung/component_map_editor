#include <QtTest>
#include <QTemporaryDir>

#include <memory>

#include "component_map_runtime/DeploymentManager.h"
#include "component_map_runtime/RuntimeCore.h"
#include "component_map_runtime/RuntimePersistence.h"

namespace
{
class PersistentHandler final : public cme::runtime::RuntimeHandler
{
public:
    QString componentType() const override { return QStringLiteral("processor"); }
    bool process(const cme::runtime::RuntimeEvent &event, QByteArray &state,
                 cme::runtime::RuntimeEvent *output,
                 cme::runtime::RuntimeFailure *) override
    {
        state.append(event.payload);
        *output = event;
        output->sourceComponentId = event.targetComponentId;
        return true;
    }
};

cme::runtime::v1::DeploymentGraph graph()
{
    cme::runtime::v1::DeploymentGraph result;
    result.set_graph_version("persistent-v1");
    auto *component = result.add_components();
    component->set_instance_id("processor-01");
    component->set_component_type("processor");
    component->set_state_schema_version(1);
    auto *port = component->add_ports();
    port->set_port_id("in");
    port->set_direction(cme::runtime::v1::PORT_DIRECTION_INPUT);
    port->set_message_type("test.event");
    return result;
}

cme::runtime::v1::RuntimeDescriptor descriptor()
{
    cme::runtime::v1::RuntimeDescriptor result;
    result.set_runtime_id("persistent-runtime");
    result.set_protocol_version("1.0");
    result.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);
    return result;
}

void registerHandler(cme::runtime::RuntimeHandlerRegistry *registry)
{
    QVERIFY(registry->registerFactory(QStringLiteral("processor"),
                                      [] { return std::make_unique<PersistentHandler>(); }));
}

} // namespace

class tst_RuntimePersistencePhase4 : public QObject
{
    Q_OBJECT

private slots:
    void stateAndHistorySurviveRestart();
    void rejectionAndLogSurviveRestart();
    void resetClearsExecutionButKeepsHistoryAndLogs();
};

void tst_RuntimePersistencePhase4::stateAndHistorySurviveRestart()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString databasePath = directory.filePath(QStringLiteral("runtime.sqlite"));

    cme::runtime::RuntimePersistence persistence;
    QString error;
    QVERIFY2(persistence.open(databasePath, QStringLiteral("persistent-runtime"), &error),
             qPrintable(error));
    cme::runtime::RuntimeService service(descriptor());
    cme::runtime::RuntimeHandlerRegistry registry;
    registerHandler(&registry);
    cme::runtime::DeploymentManager deployment(&service, &registry);
    QVERIFY(deployment.deploy(graph()).valid());
    QVERIFY(deployment.activate().valid());
    cme::runtime::RuntimeEngine engine(&service, &deployment, &registry, 4, &persistence);
    QVERIFY2(engine.start(&error), qPrintable(error));

    cme::runtime::RuntimeEvent event;
    event.eventId = "persisted-event";
    event.idempotencyKey = "persisted-key";
    event.messageType = "test.event";
    event.targetComponentId = "processor-01";
    event.payload = "state-a";
    QVERIFY(engine.publish(event));
    QCOMPARE(engine.processAll(), qsizetype(1));
    QVERIFY(engine.stop(&error));
    persistence.close();

    cme::runtime::RuntimePersistence reopened;
    QVERIFY2(reopened.open(databasePath, QStringLiteral("persistent-runtime"), &error),
             qPrintable(error));
    const auto states = reopened.loadComponentStates(&error);
    QCOMPARE(states.value(QStringLiteral("processor-01")), QByteArray("state-a"));
    QCOMPARE(reopened.pendingEventCount(&error), 0);
    QCOMPARE(reopened.eventHistoryCount(&error), 1);

    cme::runtime::RuntimeService recoveredService(descriptor());
    cme::runtime::RuntimeHandlerRegistry recoveredRegistry;
    registerHandler(&recoveredRegistry);
    cme::runtime::DeploymentManager recoveredDeployment(&recoveredService, &recoveredRegistry);
    QVERIFY(recoveredDeployment.deploy(graph()).valid());
    QVERIFY(recoveredDeployment.activate().valid());
    cme::runtime::RuntimeEngine recovered(&recoveredService, &recoveredDeployment,
                                          &recoveredRegistry, 4, &reopened);
    QVERIFY2(recovered.start(&error), qPrintable(error));
    QCOMPARE(recovered.componentState(QStringLiteral("processor-01")), QByteArray("state-a"));
}

void tst_RuntimePersistencePhase4::rejectionAndLogSurviveRestart()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cme::runtime::RuntimePersistence persistence;
    QString error;
    QVERIFY(persistence.open(directory.filePath(QStringLiteral("runtime.sqlite")),
                             QStringLiteral("rejection-runtime"), &error));
    cme::runtime::RejectionRecord record{"event-1", "key-1", "processor-01", "route-1",
                                         "MAILBOX_FULL", 1};
    QVERIFY(persistence.recordRejection(record, &error));
    QVERIFY(persistence.recordLog(QStringLiteral("warning"), QStringLiteral("MAILBOX_FULL"),
                                  QStringLiteral("Mailbox is full."), &error));
    persistence.close();

    cme::runtime::RuntimePersistence reopened;
    QVERIFY(reopened.open(directory.filePath(QStringLiteral("runtime.sqlite")),
                          QStringLiteral("rejection-runtime"), &error));
    QCOMPARE(reopened.rejectionCount(&error), 1);
    QCOMPARE(reopened.logCount(&error), 1);
}

void tst_RuntimePersistencePhase4::resetClearsExecutionButKeepsHistoryAndLogs()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    cme::runtime::RuntimePersistence persistence;
    QString error;
    QVERIFY(persistence.open(directory.filePath(QStringLiteral("runtime.sqlite")),
                             QStringLiteral("reset-runtime"), &error));
    QVERIFY(persistence.saveComponentState(QStringLiteral("node-1"), QStringLiteral("processor"),
                                           QByteArray("opaque"), 1, &error));
    QVERIFY(persistence.recordEventCompletion(QStringLiteral("event-1"), true, {}, &error));
    QVERIFY(persistence.recordLog(QStringLiteral("info"), QStringLiteral("RESET"),
                                  QStringLiteral("before reset"), &error));
    QVERIFY(persistence.resetExecutionState(&error));
    QCOMPARE(persistence.loadComponentStates(&error).size(), 0);
    QCOMPARE(persistence.pendingEventCount(&error), 0);
    QCOMPARE(persistence.eventHistoryCount(&error), 1);
    QCOMPARE(persistence.logCount(&error), 1);
}

QTEST_MAIN(tst_RuntimePersistencePhase4)
#include "tst_RuntimePersistencePhase4.moc"
