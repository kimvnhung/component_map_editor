#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <base_log.h>

#include "GraphExecutionSandboxSim.h"

int main(int argc, char *argv[])
{
    LOGD("Starting GraphExecutionSandboxSim example...");
    QGuiApplication app(argc, argv);
    GraphExecutionSandboxSim sim;
    auto clock = new ClockComponent(new_id(Component_Type));
    auto printer = new PrinterComponent(new_id(Component_Type));
    auto conn1 = new Connection(new_id(Connection_Type), clock->getId(), printer->getId());
    auto conn2 = new Connection(new_id(Connection_Type), printer->getId(), clock->getId());

    sim.addComponent(clock);
    sim.addComponent(printer);
    sim.addConnection(conn1);
    sim.addConnection(conn2);

    sim.configureStartupComponent(clock->getId());

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("sandbox"), &sim);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
    []() { QCoreApplication::exit(-1); },
    Qt::QueuedConnection);
    engine.loadFromModule("ActorMailboxExample", "Main");

    return app.exec();
}