
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <componentmapeditormanager.h>
#include <extensionpackbuilder.h>
#include <extensions/providers/customizecomponenttypeprovider.h>

#include <services/ExecutionMigrationFlags.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    cme::execution::MigrationFlags::setTokenTransportEnabled(true);

    ComponentMapEditorManager *manager = ComponentMapEditorManagerBuilder()
                                         .withExtensionPackFactory(
                                                 ExtensionPackBuilder()
                                                 .withComponentProviderFactory(
                                                         utils::makeFactory<CustomizeComponentTypeProvider>()
                                                 )
                                                 .build())
                                         .build();

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
