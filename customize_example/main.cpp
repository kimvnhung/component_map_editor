#include "src/extensions/providers/customizeextensionpack.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <extensions/contracts/ExtensionContractRegistry.h>

#include <extensions/runtime/rules/RuleBackedProviders.h>
#include <extensions/runtime/rules/RuleHotReloadService.h>
#include <extensions/runtime/rules/RuleRuntimeRegistry.h>

#include <extensions/runtime/ExtensionStartupLoader.h>
#include <extensions/runtime/PropertySchemaRegistry.h>
#include <extensions/runtime/TypeRegistry.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // 1. Create the contract registry with the current core API version.
    ExtensionContractRegistry extensionContracts({1, 0, 0});

    // 2. Set up the rule-backed providers (connection policy + validation)
    //    that read their logic from a JSON rule file at runtime.
    RuleRuntimeRegistry ruleRegistry;
    RuleBackedConnectionPolicyProvider compiledConnectionPolicy(&ruleRegistry);
    RuleBackedValidationProvider       compiledValidation(&ruleRegistry);

    QString providerError;
    extensionContracts.registerConnectionPolicyProvider(&compiledConnectionPolicy, &providerError);
    extensionContracts.registerValidationProvider(&compiledValidation, &providerError);

    // 3. Start watching the rule file – changes are picked up without restart.
    const QString ruleFilePath = QString::fromUtf8(EXAMPLE_EXTENSION_RULE_FILE);
    RuleHotReloadService ruleHotReload(&ruleRegistry);
    ruleHotReload.startWatchingFile(ruleFilePath);

    // 4. Discover and load extension packs from the manifest directory.
    const QString manifestDir = QString::fromUtf8(EXAMPLE_EXTENSION_MANIFEST_DIR);
    ExtensionStartupLoader startupLoader;
    startupLoader.registerFactory("customize.workflow", []() {
        return std::make_unique<CustomizeExtensionPack>();
    });
    startupLoader.loadFromDirectory(manifestDir, extensionContracts);

    QQmlApplicationEngine engine;

    TypeRegistry typeRegistry;
    typeRegistry.rebuildFromRegistry(extensionContracts);

    engine.rootContext()->setContextProperty(QStringLiteral("customizeComponentTypeRegistry"),
                                             &typeRegistry);



    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("customize_example", "Main");

    return QCoreApplication::exec();
}
