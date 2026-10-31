#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "component_map_runtime/RuntimeGrpcServer.h"

namespace
{
class BuiltinHandler final : public cme::runtime::RuntimeHandler
{
public:
    explicit BuiltinHandler(QString type) : m_type(std::move(type)) {}
    QString componentType() const override { return m_type; }
    bool process(const cme::runtime::RuntimeEvent &event, QByteArray &state,
                 cme::runtime::RuntimeEvent *output,
                 cme::runtime::RuntimeFailure *) override
    {
        state.append(event.payload);
        *output = event;
        output->sourceComponentId = event.targetComponentId;
        return true;
    }
private:
    QString m_type;
};
}

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
    for (const QString &type : {QStringLiteral("producer"), QStringLiteral("processor"), QStringLiteral("sink")})
    {
        registry.registerFactory(type, [type] { return std::make_unique<BuiltinHandler>(type); });
    }
    cme::runtime::DeploymentManager deployment(&service, &registry);
    cme::runtime::RuntimePersistence persistence;
    QString error;
    if (!persistence.open(parser.value(databaseOption), parser.value(runtimeOption), &error))
    {
        qCritical().noquote() << error;
        return 1;
    }
    cme::runtime::RuntimeEngine engine(&service, &deployment, &registry, 64, &persistence);
    cme::runtime::RuntimeGrpcServer server(&engine, &deployment, &service, &persistence);
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
