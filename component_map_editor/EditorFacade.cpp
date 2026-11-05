#include "EditorFacade.h"

ComponentMapEditorFacade::ComponentMapEditorFacade(QObject *parent)
    : QObject(parent)
    , m_contracts(ExtensionApiVersion{1, 0, 0})
    , m_typeRegistry(this)
    , m_graph(this)
    , m_undoStack(this)
    , m_controller(this)
    , m_validationService(this)
    , m_propertySchemaRegistry(this)
    , m_dynamicOptionsRegistry(this)
    , m_exportService(this)
{
    m_controller.setGraph(&m_graph);
    m_dynamicOptionsRegistry.setGraph(&m_graph);
    m_dynamicOptionsRegistry.setPropertySchemaRegistry(&m_propertySchemaRegistry);
    m_controller.setUndoStack(&m_undoStack);
    m_controller.setTypeRegistry(&m_typeRegistry);
    m_validationService.rebuildValidationFromRegistry(m_contracts);
}

bool ComponentMapEditorFacade::registerComponentType(const IComponentTypeProvider *provider,
                                                       QString *error)
{
    if (!m_contracts.registerComponentTypeProvider(provider, error))
        return false;
    rebuildRegistries();
    return true;
}

bool ComponentMapEditorFacade::registerConnectionPolicy(const IConnectionPolicyProvider *provider,
                                                         QString *error)
{
    if (!m_contracts.registerConnectionPolicyProvider(provider, error))
        return false;
    rebuildRegistries();
    return true;
}

bool ComponentMapEditorFacade::registerValidationProvider(const IValidationProvider *provider,
                                                          QString *error)
{
    if (!m_contracts.registerValidationProvider(provider, error))
        return false;
    rebuildRegistries();
    return true;
}

bool ComponentMapEditorFacade::registerPropertySchemaProvider(
    const IPropertySchemaProvider *provider, QString *error)
{
    if (!m_contracts.registerPropertySchemaProvider(provider, error))
        return false;
    rebuildRegistries();
    return true;
}

void ComponentMapEditorFacade::replaceComponentTypesFromRuntimeCatalog(const QVariantList &descriptors)
{
    m_typeRegistry.replaceFromRuntimeCatalog(descriptors);
}

QString ComponentMapEditorFacade::createComponent(const QString &typeId, qreal x, qreal y)
{
    return m_controller.createComponent(typeId, x, y);
}

QString ComponentMapEditorFacade::cloneComponent(const QString &sourceId,
                                                  qreal offsetX,
                                                  qreal offsetY)
{
    return m_controller.duplicateComponent(sourceId, offsetX, offsetY);
}

bool ComponentMapEditorFacade::removeComponent(const QString &componentId)
{
    if (!m_graph.componentById(componentId))
        return false;
    m_undoStack.pushRemoveComponent(&m_graph, componentId);
    return m_graph.componentById(componentId) == nullptr;
}

bool ComponentMapEditorFacade::setComponentProperty(const QString &componentId,
                                                     const QString &propertyName,
                                                     const QVariant &value)
{
    auto *component = m_graph.componentById(componentId);
    if (!component || propertyName.trimmed().isEmpty())
        return false;
    m_undoStack.pushSetComponentProperty(component, propertyName, value);
    return component->dynamicPropertyValue(propertyName) == value;
}

QString ComponentMapEditorFacade::connectPorts(const QString &sourceId, const QString &targetId)
{
    return m_controller.connectComponents(sourceId, targetId);
}

bool ComponentMapEditorFacade::validateGraph()
{
    return m_validationService.validate(&m_graph);
}

void ComponentMapEditorFacade::rebuildRegistries()
{
    m_typeRegistry.rebuildFromRegistry(m_contracts);
    m_propertySchemaRegistry.rebuildFromRegistry(m_contracts);
    m_dynamicOptionsRegistry.rebuildFromRegistry(m_contracts);
    m_validationService.rebuildValidationFromRegistry(m_contracts);
}
