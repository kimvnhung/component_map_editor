#ifndef COMPONENT_MAP_EDITOR_EDITOR_FACADE_H
#define COMPONENT_MAP_EDITOR_EDITOR_FACADE_H

#include "extensions/contracts/ExtensionContractRegistry.h"
#include "extensions/runtime/TypeRegistry.h"
#include "extensions/runtime/PropertySchemaRegistry.h"
#include "extensions/runtime/DynamicOptionsRegistry.h"
#include "models/GraphModel.h"
#include "commands/UndoStack.h"
#include "services/GraphEditorController.h"
#include "services/ValidationService.h"
#include "services/ExportService.h"

#include <QObject>
#include <QVariant>

/**
 * Stable application-facing facade for graph authoring.
 *
 * The facade owns the editor authoring objects and exposes only generic graph
 * operations. Component providers are registered here; runtime handlers and
 * runtime state remain outside the editor library.
 */
class ComponentMapEditorFacade final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(GraphModel *graph READ graph CONSTANT)
    Q_PROPERTY(TypeRegistry *typeRegistry READ typeRegistry CONSTANT)
    Q_PROPERTY(UndoStack *undoStack READ undoStack CONSTANT)
    Q_PROPERTY(GraphEditorController *controller READ controller CONSTANT)
    Q_PROPERTY(ValidationService *validationService READ validationService CONSTANT)
    Q_PROPERTY(PropertySchemaRegistry *propertySchemaRegistry READ propertySchemaRegistry CONSTANT)
    Q_PROPERTY(DynamicOptionsRegistry *dynamicOptionsRegistry READ dynamicOptionsRegistry CONSTANT)
    Q_PROPERTY(ExportService *exportService READ exportService CONSTANT)

public:
    explicit ComponentMapEditorFacade(QObject *parent = nullptr);

    GraphModel *graph() { return &m_graph; }
    TypeRegistry *typeRegistry() { return &m_typeRegistry; }
    UndoStack *undoStack() { return &m_undoStack; }
    GraphEditorController *controller() { return &m_controller; }
    ValidationService *validationService() { return &m_validationService; }
    PropertySchemaRegistry *propertySchemaRegistry() { return &m_propertySchemaRegistry; }
    DynamicOptionsRegistry *dynamicOptionsRegistry() { return &m_dynamicOptionsRegistry; }
    ExportService *exportService() { return &m_exportService; }

    Q_INVOKABLE bool registerComponentType(const IComponentTypeProvider *provider,
                                            QString *error = nullptr);
    Q_INVOKABLE bool registerConnectionPolicy(const IConnectionPolicyProvider *provider,
                                               QString *error = nullptr);
    Q_INVOKABLE bool registerValidationProvider(const IValidationProvider *provider,
                                                QString *error = nullptr);
    Q_INVOKABLE bool registerPropertySchemaProvider(const IPropertySchemaProvider *provider,
                                                     QString *error = nullptr);
    Q_INVOKABLE QString createComponent(const QString &typeId, qreal x, qreal y);
    Q_INVOKABLE QString cloneComponent(const QString &sourceId,
                                       qreal offsetX = 40.0,
                                       qreal offsetY = 40.0);
    Q_INVOKABLE bool removeComponent(const QString &componentId);
    Q_INVOKABLE bool setComponentProperty(const QString &componentId,
                                          const QString &propertyName,
                                          const QVariant &value);
    Q_INVOKABLE QString connectPorts(const QString &sourceId, const QString &targetId);
    Q_INVOKABLE bool validateGraph();

private:
    void rebuildRegistries();

    ExtensionContractRegistry m_contracts;
    TypeRegistry m_typeRegistry;
    GraphModel m_graph;
    UndoStack m_undoStack;
    GraphEditorController m_controller;
    ValidationService m_validationService;
    PropertySchemaRegistry m_propertySchemaRegistry;
    DynamicOptionsRegistry m_dynamicOptionsRegistry;
    ExportService m_exportService;
};

#endif // COMPONENT_MAP_EDITOR_EDITOR_FACADE_H
