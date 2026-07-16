
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <componentmapeditormanager.h>
#include <extensionpackbuilder.h>
#include <utils/extensioncontractregistrybuilder.h>

#include <services/ExecutionMigrationFlags.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    cme::execution::MigrationFlags::setTokenTransportEnabled(true);

    ComponentMapEditorManager *manager = nullptr;

    try
    {
        manager = ComponentMapEditorManagerBuilder()
                  .withExtensionContractRegistry(
                      ExtensionContractRegistryBuilder()
                      .build()
                  )
                  .build();
    }
    catch (std::runtime_error &e)
    {
        qCritical() << "Failed to initialize ComponentMapEditorManager:" << e.what();
        return -1;
    }

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("editorManager"), manager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
    []() { QCoreApplication::exit(-1); },
    Qt::QueuedConnection);
    engine.loadFromModule("customize_example", "Main");

    return QCoreApplication::exec();
}
