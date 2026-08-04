#include <base_log.h>
#include "GraphExecutionSandboxSim.h"

int main()
{
    LOGD("Starting GraphExecutionSandboxSim example...");
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

    sim.execute();
    return 0;
}