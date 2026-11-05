#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "component_map_runtime/RuntimeGrpcServer.h"
#include "component_map_runtime/BuiltinFactoryExtension.h"
#include "component_map_runtime/ComponentCatalog.h"
#include "component_map_runtime/ComponentDiscoveryTypes.h"

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption addressOption(QStringList() << "a" << "address", "gRPC address.", "address", "127.0.0.1:50051");
    QCommandLineOption databaseOption(QStringList() << "d" << "database", "SQLite database path.", "path", "component_map_runtime.sqlite");
    QCommandLineOption runtimeOption(QStringList() << "r" << "runtime-id", "Runtime ID.", "id", "component-map-runtime");
    parser.addOption(addressOption);
    parser.addOption(databaseOption);
    parser.addOption(runtimeOption);
    parser.process(application);

    cme::runtime::v1::RuntimeDescriptor descriptor;
    descriptor.set_runtime_id(parser.value(runtimeOption).toStdString());
    descriptor.set_protocol_version("1.0");
    descriptor.set_mode(cme::runtime::v1::RUNTIME_MODE_BACKGROUND);
    cme::runtime::RuntimeService service(descriptor);
    cme::runtime::RuntimeHandlerRegistry registry;
    cme::runtime::ComponentCatalog catalog;
    QString error;
    if (!registry.registerExtension(cme::runtime::makeBuiltinFactoryExtension(), &error))
    {
        qCritical().noquote() << error;
        return 1;
    }
    for (const auto &componentType : registry.componentTypes())
    {
        const auto componentDescriptor = registry.descriptor(componentType);
        if (!catalog.registerDescriptor(componentDescriptor, nullptr))
        {
            qCritical().noquote() << "Unable to register built-in descriptor:" << componentType;
            return 1;
        }
    }
    cme::runtime::DeploymentManager deployment(&service, &registry, &catalog);
    cme::runtime::RuntimePersistence persistence;
    if (!persistence.open(parser.value(databaseOption), parser.value(runtimeOption), &error))
    {
        qCritical().noquote() << error;
        return 1;
    }
    cme::runtime::RuntimeEngine engine(&service, &deployment, &registry, 64, &persistence);
    engine.setBackgroundExecution(true, 1);
    cme::runtime::RuntimeGrpcServer server(&engine, &deployment, &service, &persistence, &catalog);
    if (!server.start(parser.value(addressOption), &error))
    {
        qCritical().noquote() << error;
        return 1;
    }
    qInfo().noquote() << "component_map_runtime listening on" << parser.value(addressOption);
    application.exec();
    server.shutdown();
    return 0;
}
