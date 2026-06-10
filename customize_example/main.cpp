
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <componentmapeditormanager.h>

#include <services/ExecutionMigrationFlags.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    cme::execution::MigrationFlags::setTokenTransportEnabled(true);

    ComponentMapEditorManager manager;

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("editorManager"), &manager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
    []() { QCoreApplication::exit(-1); },
    Qt::QueuedConnection);
    engine.loadFromModule("customize_example", "Main");

    return QCoreApplication::exec();
}
