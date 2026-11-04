cmake_minimum_required(VERSION 3.14)

project(cme_editor_ui_module VERSION 1.0.0 LANGUAGES CXX)
if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()
find_package(Qt6 REQUIRED COMPONENTS Core Quick)

# Explicit ownership keeps the reusable editor/QML module separate from the runtime.
set(COMPONENT_MAP_EDITOR_HEADERS
    adapters/AdapterCommon.h
    adapters/GraphAdapter.h
    adapters/PolicyAdapter.h
    adapters/ValidationAdapter.h
    commands/GraphCommand.h
    commands/GraphCommands.h
    commands/UndoStack.h
    componentmapeditormanager.h
    EditorFacade.h
    extensions/common.h
    extensions/contracts/ExtensionApiVersion.h
    extensions/contracts/ExtensionContractRegistry.h
    extensions/contracts/ExtensionManifest.h
    extensions/contracts/IActionProvider.h
    extensions/contracts/IComponentTypeProvider.h
    extensions/contracts/IConnectionPolicyProvider.h
    extensions/contracts/IExecutionSemanticsProvider.h
    extensions/contracts/IExtensionPack.h
    extensions/contracts/IPropertySchemaProvider.h
    extensions/contracts/IValidationProvider.h
    extensions/contracts/builders/ComponentTypeProviderBuilder.h
    extensions/contracts/builders/ExtensionContractRegistryBuilder.h
    extensions/contracts/builders/ExtensionPackBuilder.h
    extensions/runtime/DynamicOptionsRegistry.h
    extensions/runtime/ExtensionCompatibilityChecker.h
    extensions/runtime/ExtensionManifestJson.h
    extensions/runtime/ExtensionStartupLoader.h
    extensions/runtime/GraphPropertySchemaProvider.h
    extensions/runtime/PropertySchemaRegistry.h
    extensions/runtime/PublicApiContractAdapter.h
    extensions/runtime/SchemaFieldDefinition.h
    extensions/runtime/SchemaOptionsSpec.h
    extensions/runtime/SchemaSectionModel.h
    extensions/runtime/TypeRegistry.h
    extensions/runtime/rules/RuleBackedProviders.h
    extensions/runtime/rules/RuleCompiler.h
    extensions/runtime/rules/RuleDslTypes.h
    extensions/runtime/rules/RuleHotReloadService.h
    extensions/runtime/rules/RuleRuntimeEngine.h
    extensions/runtime/rules/RuleRuntimeRegistry.h
    extensions/runtime/templates/ComponentTypeTemplateAdapter.h
    extensions/runtime/templates/ConnectionPolicyTemplateAdapter.h
    extensions/runtime/templates/PropertySchemaTemplateAdapter.h
    extensions/runtime/templates/TemplateProtoHelpers.h
    extensions/runtime/templates/customizeexecutioncommon.h
    extensions/sample_pack/SampleActionProvider.h
    extensions/sample_pack/SampleComponentTypeProvider.h
    extensions/sample_pack/SampleConnectionPolicyProvider.h
    extensions/sample_pack/SampleExecutionSemanticsProvider.h
    extensions/sample_pack/SampleExtensionPack.h
    extensions/sample_pack/SamplePropertySchemaProvider.h
    extensions/sample_pack/SampleValidationProvider.h
    extensions/sample_pack/built_in_validation_providers/WorkflowConnectionValidationProvider.h
    extensions/sample_pack/built_in_validation_providers/WorkflowStructureValidationProvider.h
    extensions/sample_pack/built_in_validation_providers/WorkflowTopologyValidationProvider.h
    extensions/sample_pack/built_in_validation_providers/WorkflowValidationProvider.h
    models/ComponentModel.h
    models/ConnectionModel.h
    models/GraphModel.h
    models/TimelineModel.h
    routing/OrthogonalHeuristicStrategy.h
    routing/RouteTypes.h
    routing/RoutingEngine.h
    routing/RoutingHelpers.h
    services/DeploymentGraphExporter.h
    services/ExportService.h
    services/GraphEditorController.h
    services/GraphSchema.h
    services/InvariantChecker.h
    services/MemoryMonitor.h
    services/PerformanceBudgets.h
    services/TraversalEngine.h
    services/ValidationService.h
    transport/GrpcTemplateDescriptorTransport.h
    ui/GraphViewportItem.h
    ui/rendering/ComponentRenderPass.h
    ui/rendering/ConnectionRenderPass.h
    ui/rendering/GridRenderPass.h
    ui/rendering/LabelRenderPass.h
    ui/rendering/LabelTextureBuilder.h
    ui/rendering/LabelTextureNode.h
    utils/GraphHelper.h
    utils/PropertySchemaHelper.h
    utils/common.h
)

set(COMPONENT_MAP_EDITOR_SOURCES
    adapters/AdapterCommon.cpp
    adapters/GraphAdapter.cpp
    adapters/PolicyAdapter.cpp
    adapters/ValidationAdapter.cpp
    commands/GraphCommand.cpp
    commands/GraphCommands.cpp
    commands/UndoStack.cpp
    componentmapeditormanager.cpp
    EditorFacade.cpp
    extensions/common.cpp
    extensions/contracts/ExtensionApiVersion.cpp
    extensions/contracts/ExtensionContractRegistry.cpp
    extensions/contracts/ExtensionManifest.cpp
    extensions/contracts/IValidationProvider.cpp
    extensions/contracts/builders/ComponentTypeProviderBuilder.cpp
    extensions/contracts/builders/ExtensionContractRegistryBuilder.cpp
    extensions/contracts/builders/ExtensionPackBuilder.cpp
    extensions/runtime/DynamicOptionsRegistry.cpp
    extensions/runtime/ExtensionCompatibilityChecker.cpp
    extensions/runtime/ExtensionManifestJson.cpp
    extensions/runtime/ExtensionStartupLoader.cpp
    extensions/runtime/GraphPropertySchemaProvider.cpp
    extensions/runtime/PropertySchemaRegistry.cpp
    extensions/runtime/PublicApiContractAdapter.cpp
    extensions/runtime/SchemaFieldDefinition.cpp
    extensions/runtime/SchemaOptionsSpec.cpp
    extensions/runtime/SchemaSectionModel.cpp
    extensions/runtime/TypeRegistry.cpp
    extensions/runtime/rules/RuleBackedProviders.cpp
    extensions/runtime/rules/RuleCompiler.cpp
    extensions/runtime/rules/RuleHotReloadService.cpp
    extensions/runtime/rules/RuleRuntimeEngine.cpp
    extensions/runtime/rules/RuleRuntimeRegistry.cpp
    extensions/runtime/templates/ComponentTypeTemplateAdapter.cpp
    extensions/runtime/templates/ConnectionPolicyTemplateAdapter.cpp
    extensions/runtime/templates/PropertySchemaTemplateAdapter.cpp
    extensions/runtime/templates/TemplateProtoHelpers.cpp
    extensions/runtime/templates/customizeexecutioncommon.cpp
    extensions/sample_pack/SampleActionProvider.cpp
    extensions/sample_pack/SampleComponentTypeProvider.cpp
    extensions/sample_pack/SampleConnectionPolicyProvider.cpp
    extensions/sample_pack/SampleExecutionSemanticsProvider.cpp
    extensions/sample_pack/SampleExtensionPack.cpp
    extensions/sample_pack/SamplePropertySchemaProvider.cpp
    extensions/sample_pack/SampleValidationProvider.cpp
    extensions/sample_pack/built_in_validation_providers/WorkflowConnectionValidationProvider.cpp
    extensions/sample_pack/built_in_validation_providers/WorkflowStructureValidationProvider.cpp
    extensions/sample_pack/built_in_validation_providers/WorkflowTopologyValidationProvider.cpp
    extensions/sample_pack/built_in_validation_providers/WorkflowValidationProvider.cpp
    models/ComponentModel.cpp
    models/ConnectionModel.cpp
    models/GraphModel.cpp
    models/TimelineModel.cpp
    routing/OrthogonalHeuristicStrategy.cpp
    routing/RoutingEngine.cpp
    routing/RoutingHelpers.cpp
    services/DeploymentGraphExporter.cpp
    services/ExportService.cpp
    services/GraphEditorController.cpp
    services/GraphSchema.cpp
    services/InvariantChecker.cpp
    services/MemoryMonitor.cpp
    services/PerformanceBudgets.cpp
    services/TraversalEngine.cpp
    services/ValidationService.cpp
    transport/GrpcTemplateDescriptorTransport.cpp
    ui/GraphViewportItem.cpp
    ui/rendering/ComponentRenderPass.cpp
    ui/rendering/ConnectionRenderPass.cpp
    ui/rendering/GridRenderPass.cpp
    ui/rendering/LabelRenderPass.cpp
    ui/rendering/LabelTextureBuilder.cpp
    utils/GraphHelper.cpp
    utils/PropertySchemaHelper.cpp
)

# Editor core owns graph models, authoring services, contracts, and facade.
add_library(cme_editor_core STATIC
    ${COMPONENT_MAP_EDITOR_HEADERS}
    ${COMPONENT_MAP_EDITOR_SOURCES}
)

set(COMPONENT_MAP_EDITOR_QML
    ui/qml/ComponentItem.qml
    ui/qml/Connection.qml
    ui/qml/ExecutionPanel.qml
    ui/qml/GraphCanvas.qml
    ui/qml/GraphStatusBar.qml
    ui/qml/InteractionStateManager.qml
    ui/qml/Palette.qml
    ui/qml/PerformanceTelemetry.qml
    ui/qml/PropertyPanel.qml
    ui/qml/SchemaFormRenderer.qml
    ui/qml/components/ConnectionHandler.qml
    ui/qml/components/GraphIssueOverlay.qml
    ui/qml/components/ReadOnlyPlainText.qml
    ui/qml/components/ResizableItem.qml
    ui/qml/items/Arrow.qml
    ui/qml/items/TimelineItem.qml
    ui/qml/panels/TimelinePanel.qml
    ui/qml/panels/RuntimeOverviewPanel.qml
    ui/qml/panels/DeploymentPanel.qml
    ui/qml/panels/ComponentStatusPanel.qml
    ui/qml/panels/EventHistoryPanel.qml
    ui/qml/panels/RuntimeLogPanel.qml
    ui/qml/panels/RejectionPanel.qml
)

set(COMPONENT_MAP_EDITOR_JS
)

# Runtime lifecycle/query transport is a separate boundary from the editor.
add_library(cme_runtime_client STATIC
    services/RuntimeClient.h
    services/RuntimeClient.cpp
)

target_include_directories(cme_runtime_client
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/services
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_BINARY_DIR}/proto/runtime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cme_runtime_client
    PUBLIC
        Qt6::Core
        cme_runtime_proto
        gRPC::grpc++
)

set_target_properties(cme_runtime_client PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

qt_add_qml_module(cme_editor_ui_module
    URI ComponentMapEditor
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ComponentMapEditor"
    IMPORTS
        QmlFontAwesome
    TYPEINFO
        "ComponentMapEditor.qmltypes"
    QML_FILES
        ${COMPONENT_MAP_EDITOR_QML}
        ${COMPONENT_MAP_EDITOR_JS}
    SOURCES
        ${COMPONENT_MAP_EDITOR_HEADERS}
)

target_link_libraries(cme_editor_core
    PUBLIC
        Qt6::Core
        Qt6::Quick
        cme_proto
        cme_runtime_proto
        cme_runtime_client
        base_log
        QmlFontAwesome::QmlFontAwesome
)

target_link_libraries(cme_editor_ui_module
    PUBLIC
        cme_editor_core
        Qt6::Core
        Qt6::Quick
        cme_proto
        cme_runtime_proto
        cme_runtime_client
        base_log

    PRIVATE
        QmlFontAwesome::QmlFontAwesome
)

# QML target depends on core but never owns core implementation sources.
add_library(cme_editor_qml INTERFACE)
target_link_libraries(cme_editor_qml INTERFACE cme_editor_ui_module cme_editor_core)

# Copy public headers to the generated include directory.
foreach(HEADER_FILE ${COMPONENT_MAP_EDITOR_HEADERS})
    get_filename_component(HEADER_FILE_NAME ${HEADER_FILE} NAME)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_FILE}
        ${CMAKE_CURRENT_BINARY_DIR}/include/${HEADER_FILE_NAME}
        COPYONLY)
endforeach()

target_include_directories(cme_editor_core
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/proto
)

target_include_directories(cme_editor_ui_module
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/proto
)

target_compile_definitions(cme_editor_ui_module
    PRIVATE
        LOG_TAG="ComponentMapEditor"
)

# Optional compatibility target for the legacy local execution simulator.
# The production editor target intentionally does not compile or link these files.
add_library(cme_editor_compat STATIC)

set(CME_EDITOR_COMPAT_HEADERS
    adapters/ExecutionAdapter.h
    compatibility/builders/ExecutionPlanBuilder.h
    compatibility/extensions/CompositeExecutionProvider.h
    compatibility/ExecutionMigrationFlags.h
    preview/GraphExecutionSandbox.h
    compatibility/execution_internal/ActorMailboxExecutionEngine.h
    compatibility/execution_internal/ExecutionContext.h
    compatibility/execution_internal/ExecutionPlan.h
    compatibility/execution_internal/ExecutionRequest.h
    compatibility/execution_internal/IExecutionEngine.h
    compatibility/execution_internal/ProviderResultValidation.h
    compatibility/execution_internal/SequentialExecutionEngine.h
    compatibility/execution_internal/actor_mailbox/Actor.h
    compatibility/execution_internal/actor_mailbox/ActorRegistry.h
    compatibility/execution_internal/actor_mailbox/ActorScheduler.h
    compatibility/execution_internal/actor_mailbox/GraphSnapshotActorAdapter.h
    compatibility/execution_internal/actor_mailbox/Mailbox.h
)

set(CME_EDITOR_COMPAT_SOURCES
    adapters/ExecutionAdapter.cpp
    compatibility/builders/ExecutionPlanBuilder.cpp
    compatibility/extensions/CompositeExecutionProvider.cpp
    compatibility/ExecutionMigrationFlags.cpp
    preview/GraphExecutionSandbox.cpp
    compatibility/execution_internal/ActorMailboxExecutionEngine.cpp
    compatibility/execution_internal/ProviderResultValidation.cpp
    compatibility/execution_internal/SequentialExecutionEngine.cpp
    compatibility/execution_internal/actor_mailbox/Actor.cpp
    compatibility/execution_internal/actor_mailbox/ActorRegistry.cpp
    compatibility/execution_internal/actor_mailbox/ActorScheduler.cpp
    compatibility/execution_internal/actor_mailbox/GraphSnapshotActorAdapter.cpp
    compatibility/execution_internal/actor_mailbox/Mailbox.cpp
)

target_sources(cme_editor_compat PRIVATE
    ${CME_EDITOR_COMPAT_HEADERS}
    ${CME_EDITOR_COMPAT_SOURCES}
)

target_include_directories(cme_editor_compat
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_BINARY_DIR}/proto
        ${CMAKE_BINARY_DIR}/proto/runtime
)

target_link_libraries(cme_editor_compat
    PUBLIC
        cme_editor_ui_module
        cme_proto
        cme_runtime_proto
        Qt6::Core
)

target_compile_definitions(cme_editor_compat
    PRIVATE
        LOG_TAG="ComponentMapEditorCompat"
)

set_target_properties(cme_editor_compat PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# Keep simulator code available only to explicitly opted-in compatibility users.
add_library(cme_editor_compat_alias ALIAS cme_editor_compat)
