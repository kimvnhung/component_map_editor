#include <QGuiApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/ExtensionStartupLoader.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/rules/RuleBackedProviders.h"
#include "extensions/runtime/rules/RuleHotReloadService.h"
#include "extensions/runtime/rules/RuleRuntimeRegistry.h"
#include "extensions/sample_pack/SampleExtensionPack.h"
#include "services/GraphExecutionSandbox.h"
#include "services/ValidationService.h"

#ifndef EXAMPLE_EXTENSION_MANIFEST_DIR
#define EXAMPLE_EXTENSION_MANIFEST_DIR ""
#endif

#ifndef EXAMPLE_EXTENSION_RULE_FILE
#define EXAMPLE_EXTENSION_RULE_FILE ""
#endif

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    ExtensionContractRegistry extensionContracts({1, 0, 0});

    RuleRuntimeRegistry ruleRegistry;
    RuleBackedConnectionPolicyProvider compiledConnectionPolicy(&ruleRegistry);
    RuleBackedValidationProvider compiledValidation(&ruleRegistry);
    QString providerError;
    if (!extensionContracts.registerConnectionPolicyProvider(&compiledConnectionPolicy, &providerError)) {
        qWarning().noquote() << "[Rules][ERROR] Failed to register compiled connection policy provider:" << providerError;
    }
    if (!extensionContracts.registerValidationProvider(&compiledValidation, &providerError)) {
        qWarning().noquote() << "[Rules][ERROR] Failed to register compiled validation provider:" << providerError;
    }

    RuleHotReloadService ruleHotReload(&ruleRegistry);
    const QString ruleFilePath = QString::fromUtf8(EXAMPLE_EXTENSION_RULE_FILE);
    if (!ruleHotReload.startWatchingFile(ruleFilePath)) {
        const QVector<RuleDiagnostic> diagnostics = ruleHotReload.lastDiagnostics();
        if (!diagnostics.isEmpty()) {
            const RuleDiagnostic first = diagnostics.first();
            qWarning().noquote() << "[Rules][ERROR]" << first.message
                                 << "file=" << first.location.filePath
                                 << "jsonPath=" << first.location.jsonPath
                                 << "line=" << first.location.line
                                 << "column=" << first.location.column;
        }
    }

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

    GraphExecutionSandbox executionSandbox;
    executionSandbox.rebuildSemanticsFromRegistry(extensionContracts);

    ValidationService validationService;
    validationService.rebuildValidationFromRegistry(extensionContracts);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("startupPropertySchemaRegistry"),
                                             &propertySchemas);
    engine.rootContext()->setContextProperty(QStringLiteral("startupExecutionSandbox"),
                                             &executionSandbox);
    engine.rootContext()->setContextProperty(QStringLiteral("startupValidationService"),
                                             &validationService);
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
