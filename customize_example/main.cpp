
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <componentmapeditormanager.h>
#include <EditorFacade.h>
#include <extensions/providers/customizecomponenttypeprovider.h>
#include <extensions/providers/customizeconnectionpolicyprovider.h>
#include <extensions/providers/customizepropertyschemaprovider.h>
#include <extensions/providers/customizevalidationprovider.h>

#include <extensions/contracts/builders/ExtensionContractRegistryBuilder.h>
#include <extensions/contracts/builders/ExtensionPackBuilder.h>
#include <extensions/contracts/IPropertySchemaProvider.h>
#include <extensions/contracts/IConnectionPolicyProvider.h>
#include <extensions/contracts/IValidationProvider.h>

#include <QDebug>
#include <memory>
#include <stdexcept>

#ifndef EXAMPLE_EXTENSION_MANIFEST_DIR
#define EXAMPLE_EXTENSION_MANIFEST_DIR ""
#endif

#ifndef EXAMPLE_EXTENSION_RULE_FILE
#define EXAMPLE_EXTENSION_RULE_FILE ""
#endif

#ifndef COMPONENT_TYPES_DEFINITION_FILE
#define COMPONENT_TYPES_DEFINITION_FILE ""
#endif

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    ComponentMapEditorFacade facade;
    CustomizeComponentTypeProvider componentTypes;
    CustomizeConnectionPolicyProvider connectionPolicies;
    CustomizePropertySchemaProvider propertySchemas;
    CustomizeValidationProvider validation;
    QString providerError;
    if (!facade.registerComponentType(&componentTypes, &providerError)
        || !facade.registerConnectionPolicy(&connectionPolicies, &providerError)
        || !facade.registerPropertySchemaProvider(&propertySchemas, &providerError)
        || !facade.registerValidationProvider(&validation, &providerError))
    {
        qCritical() << "Failed to initialize editor facade:" << providerError;
        return -1;
    }

    std::unique_ptr<ComponentMapEditorManager> manager;
    try
    {
        manager = ComponentMapEditorManagerBuilder()
                      .withExtensionContractRegistry(ExtensionContractRegistryBuilder().build())
                      .withRuleFilePath(EXAMPLE_EXTENSION_RULE_FILE)
                      .withManifestDirectory(EXAMPLE_EXTENSION_MANIFEST_DIR)
                      .build();
    }
    catch (const std::runtime_error& error)
    {
        qCritical() << "Failed to initialize runtime client:" << error.what();
        return -1;
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("editorFacade"), &facade);
    engine.rootContext()->setContextProperty(QStringLiteral("editorManager"), manager.get());

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("customize_example", "Main");

    return QCoreApplication::exec();
}

/** The example app keeps runtime transport separate from editor authoring. */
// The runtime manager is exposed only for deployment and monitoring operations.
// All graph authoring providers and models are owned by editorFacade above.
