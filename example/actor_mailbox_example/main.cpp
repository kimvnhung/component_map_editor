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
    auto appleProducer = new FruitProducerComponent(new_id(Component_Type), "apple", 1, 0.2);
    auto bananaProducer = new FruitProducerComponent(new_id(Component_Type), "banana", 2, 0.1);
    auto orangeProducer = new FruitProducerComponent(new_id(Component_Type), "orange", 3, 0.25);
    auto store = new StoreComponent(new_id(Component_Type), "Store");
    auto empl1 = new EmployeeComponent(new_id(Component_Type), "Employee 1",
    {
        {"apple", 2},
        {"banana", 1},
        {"orange", 3}
    });
    auto empl2 = new EmployeeComponent(new_id(Component_Type), "Employee 2",
    {
        {"apple", 1},
        {"banana", 2},
        {"orange", 1}
    });
    auto empl3 = new EmployeeComponent(new_id(Component_Type), "Employee 3",
    {
        {"apple", 3},
        {"banana", 1},
        {"orange", 2}
    });
    auto seller = new SellerComponent(new_id(Component_Type), "Seller",
    {
        {"apple", 0.1},
        {"banana", 0.02},
        {"orange", 0.05}
    },
    {
        {"apple", 5},
        {"banana", 7},
        {"orange", 15}
    });
    auto manager = new ManagerComponent(new_id(Component_Type), "Manager");

    auto manager2apple = new Connection(new_id(Connection_Type), manager->getId(), appleProducer->getId());
    auto apple2manager = new Connection(new_id(Connection_Type), appleProducer->getId(), manager->getId());
    auto manager2banana = new Connection(new_id(Connection_Type), manager->getId(), bananaProducer->getId());
    auto banana2manager = new Connection(new_id(Connection_Type), bananaProducer->getId(), manager->getId());
    auto manager2orange = new Connection(new_id(Connection_Type), manager->getId(), orangeProducer->getId());
    auto orange2manager = new Connection(new_id(Connection_Type), orangeProducer->getId(), manager->getId());
    auto apple2store = new Connection(new_id(Connection_Type), appleProducer->getId(), store->getId());
    auto banana2store = new Connection(new_id(Connection_Type), bananaProducer->getId(), store->getId());
    auto orange2store = new Connection(new_id(Connection_Type), orangeProducer->getId(), store->getId());
    auto store2empl1 = new Connection(new_id(Connection_Type), store->getId(), empl1->getId());
    auto store2empl2 = new Connection(new_id(Connection_Type), store->getId(), empl2->getId());
    auto store2empl3 = new Connection(new_id(Connection_Type), store->getId(), empl3->getId());
    auto empl12store = new Connection(new_id(Connection_Type), empl1->getId(), store->getId());
    auto empl22store = new Connection(new_id(Connection_Type), empl2->getId(), store->getId());
    auto empl32store = new Connection(new_id(Connection_Type), empl3->getId(), store->getId());
    auto store2seller = new Connection(new_id(Connection_Type), store->getId(), seller->getId());
    auto seller2manager = new Connection(new_id(Connection_Type), seller->getId(), manager->getId());

    sim.addComponent(appleProducer);
    // sim.addComponent(bananaProducer);
    // sim.addComponent(orangeProducer);
    // sim.addComponent(store);
    // sim.addComponent(empl1);
    // sim.addComponent(empl2);
    // sim.addComponent(empl3);
    // sim.addComponent(seller);
    sim.addComponent(manager);

    sim.addConnection(manager2apple);
    sim.addConnection(apple2manager);
    // sim.addConnection(manager2banana);
    // sim.addConnection(manager2orange);
    // sim.addConnection(apple2store);
    // sim.addConnection(banana2store);
    // sim.addConnection(orange2store);
    // sim.addConnection(store2empl1);
    // sim.addConnection(store2empl2);
    // sim.addConnection(store2empl3);
    // sim.addConnection(empl12store);
    // sim.addConnection(empl22store);
    // sim.addConnection(empl32store);
    // sim.addConnection(store2seller);
    // sim.addConnection(seller2manager);

    sim.configureStartupComponent(manager->getId(), {{"init_budget", "100"}});

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