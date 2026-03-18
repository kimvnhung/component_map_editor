#include <QGuiApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/sample_pack/SampleExtensionPack.h"

#ifndef EXAMPLE_EXTENSION_MANIFEST_DIR
#define EXAMPLE_EXTENSION_MANIFEST_DIR ""
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    ExtensionContractRegistry extensionContracts({1, 0, 0});
    ExtensionStartupLoader startupLoader;
    startupLoader.registerFactory(QStringLiteral("sample.workflow"), []() {
        return std::make_unique<SampleExtensionPack>();
    });

    const QString manifestDir = QString::fromUtf8(EXAMPLE_EXTENSION_MANIFEST_DIR);
    const ExtensionLoadResult loadResult = startupLoader.loadFromDirectory(manifestDir, extensionContracts);
    for (const ExtensionLoadDiagnostic &diag : loadResult.diagnostics) {
        if (diag.severity == ExtensionLoadDiagnostic::Severity::Error) {
            qWarning().noquote() << "[ExtensionStartupLoader][ERROR]" << diag.message
                                 << "extensionId=" << diag.extensionId
                                 << "manifest=" << diag.manifestPath;
        } else {
            qInfo().noquote() << "[ExtensionStartupLoader]" << diag.message
                              << "extensionId=" << diag.extensionId
                              << "manifest=" << diag.manifestPath;
        }
    }

    PropertySchemaRegistry propertySchemas;
    propertySchemas.rebuildFromRegistry(extensionContracts);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("startupPropertySchemaRegistry"),
                                             &propertySchemas);
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
