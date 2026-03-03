#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    // DO NOT CHANGE THIS LINE - MODULE_NAME AND STARTUP_FILE ARE DEFINED IN CMAKE
    engine.loadFromModule("ExampleApp", "Main");
#else
    // Load QML main file
    const QUrl url(QStringLiteral("qrc:/qt/qml/plugin_manager/Main.qml"));
    engine.load(url);
#endif

    return app.exec();
}
