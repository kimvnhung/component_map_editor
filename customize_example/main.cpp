
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <componentmapeditormanager.h>
#include <extensions/contracts/builders/ExtensionPackBuilder.h>
#include <extensions/contracts/builders/ExtensionContractRegistryBuilder.h>
#include <extensions/sample_pack/SampleComponentTypeProvider.h>

#include "extensions/factory_pack/providers/FactoryComponentTypeProvider.h"
#include "extensions/factory_pack/providers/FactoryPropertySchemaProvider.h"
#include "extensions/providers/customizecomponenttypeprovider.h"
#include "extensions/providers/customizepropertyschemaprovider.h"


#ifndef EXAMPLE_EXTENSION_MANIFEST_DIR
#define EXAMPLE_EXTENSION_MANIFEST_DIR ""
#endif

#ifndef EXAMPLE_EXTENSION_RULE_FILE
#define EXAMPLE_EXTENSION_RULE_FILE ""
#endif

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);


    std::unique_ptr<ComponentMapEditorManager> manager;

    try
    {
        manager = ComponentMapEditorManagerBuilder()
                      .withPackFactoryEntry(
                          ExtensionPackBuilder()
                              .withExtensionId("customize.workflow")
                              .withCapabilities(static_cast<extensions::Capability>(
                                  extensions::Capability_ComponentTypes | extensions::Capability_PropertySchema))
                              .withComponentProviderFactory(utils::makeFactory<CustomizeComponentTypeProvider>())
                              .withPropertySchemaProviderFactory(utils::makeFactory<CustomizePropertySchemaProvider>())
                              .build())
                      .withManifestDirectory(EXAMPLE_EXTENSION_MANIFEST_DIR)
                      .withRuleFilePath(EXAMPLE_EXTENSION_RULE_FILE)
                      .withExtensionContractRegistry(ExtensionContractRegistryBuilder().build())
                      .build();
    }
    catch (std::runtime_error& e)
    {
        qCritical() << "Failed to initialize ComponentMapEditorManager:" << e.what();
        return -1;
    }

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("editorManager"), manager.get());

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("customize_example", "Main");

    return QCoreApplication::exec();
}
