cmake_minimum_required(VERSION 3.21)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickTest Test)

# The ComponentMapEditor QML module output lives one level above its
# auto-generated sub-folder (e.g. <build>/component_map_editor/ contains
# the ComponentMapEditor/ dir).  Pass that parent path to the test binary so
# the QML engine can resolve the import.
set(COMPONENT_MAP_EDITOR_IMPORT_DIR
    "$<TARGET_FILE_DIR:libComponentMapEditor>/../.."
    CACHE INTERNAL "")

qt_add_executable(tst_InteractionStateManager
    main_test.cpp
)

target_compile_definitions(tst_InteractionStateManager PRIVATE
    COMPONENT_MAP_EDITOR_IMPORT_DIR="${CMAKE_BINARY_DIR}/component_map_editor"
)

# Embed the QML test file as a Qt resource so QUICK_TEST_MAIN_WITH_SETUP can
# load it from the binary without needing the source tree at runtime.
qt_add_resources(tst_InteractionStateManager "qml_test_res"
    PREFIX "/"
    FILES
        tst_InteractionStateManager.qml
    tst_PaletteGestureArbitration.qml
    tst_SchemaFormRenderer.qml
)

# QuickTest's default discovery scans the current working directory for
# tst_*.qml files. Copy the file into build/tests so direct IDE launches and
# ctest both work without requiring source-dir arguments.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_InteractionStateManager.qml
    ${CMAKE_CURRENT_BINARY_DIR}/tst_InteractionStateManager.qml
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_PaletteGestureArbitration.qml
    ${CMAKE_CURRENT_BINARY_DIR}/tst_PaletteGestureArbitration.qml
    COPYONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_SchemaFormRenderer.qml
    ${CMAKE_CURRENT_BINARY_DIR}/tst_SchemaFormRenderer.qml
    COPYONLY
)

target_link_libraries(tst_InteractionStateManager
    PRIVATE
        Qt6::Quick
        Qt6::QuickTest
        libComponentMapEditor
)

add_test(
    NAME  tst_InteractionStateManager
    COMMAND tst_InteractionStateManager -input ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_PersistenceValidation
    tst_PersistenceValidation.cpp
)

target_link_libraries(tst_PersistenceValidation
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_PersistenceValidation
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_PersistenceValidation
    COMMAND tst_PersistenceValidation
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_ExtensionContractRegistry
    tst_ExtensionContractRegistry.cpp
)

target_link_libraries(tst_ExtensionContractRegistry
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_ExtensionContractRegistry
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_ExtensionContractRegistry
    COMMAND tst_ExtensionContractRegistry
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_ExtensionStartupLoader
    tst_ExtensionStartupLoader.cpp
)

target_link_libraries(tst_ExtensionStartupLoader
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_ExtensionStartupLoader
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_ExtensionStartupLoader
    COMMAND tst_ExtensionStartupLoader
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_TypeRegistry
    tst_TypeRegistry.cpp
)

target_link_libraries(tst_TypeRegistry
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_TypeRegistry
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_TypeRegistry
    COMMAND tst_TypeRegistry
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_GraphEditorController
    tst_GraphEditorController.cpp
)

target_link_libraries(tst_GraphEditorController
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_GraphEditorController
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_GraphEditorController
    COMMAND tst_GraphEditorController
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_PropertySchemaRegistry
    tst_PropertySchemaRegistry.cpp
)

target_link_libraries(tst_PropertySchemaRegistry
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_PropertySchemaRegistry
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_PropertySchemaRegistry
    COMMAND tst_PropertySchemaRegistry
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_DynamicOptionsRegistry
    tst_DynamicOptionsRegistry.cpp
)

target_link_libraries(tst_DynamicOptionsRegistry
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_DynamicOptionsRegistry
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_DynamicOptionsRegistry
    COMMAND tst_DynamicOptionsRegistry
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_RuleCompilerRuntime
    tst_RuleCompilerRuntime.cpp
)

target_link_libraries(tst_RuleCompilerRuntime
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_RuleCompilerRuntime
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_RuleCompilerRuntime
    COMMAND tst_RuleCompilerRuntime
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_TraversalEngine
    tst_TraversalEngine.cpp
)

target_link_libraries(tst_TraversalEngine
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_TraversalEngine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_TraversalEngine
    COMMAND tst_TraversalEngine
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_ExecutionPlanBuilder
    tst_ExecutionPlanBuilder.cpp
)

target_link_libraries(tst_ExecutionPlanBuilder
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_ExecutionPlanBuilder
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_ExecutionPlanBuilder
    COMMAND tst_ExecutionPlanBuilder
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_SequentialExecutionEngine
    tst_SequentialExecutionEngine.cpp
)

target_link_libraries(tst_SequentialExecutionEngine
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_SequentialExecutionEngine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_SequentialExecutionEngine
    COMMAND tst_SequentialExecutionEngine
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_ActorMailboxEngine
    tst_ActorMailboxEngine.cpp
)

target_link_libraries(tst_ActorMailboxEngine
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_ActorMailboxEngine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_ActorMailboxEngine
    COMMAND tst_ActorMailboxEngine
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_ActorEngineIntegration
    tst_ActorEngineIntegration.cpp
)

target_link_libraries(tst_ActorEngineIntegration
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_ActorEngineIntegration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_ActorEngineIntegration
    COMMAND tst_ActorEngineIntegration
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_CompatibilityMigrationToolkit
    tst_CompatibilityMigrationToolkit.cpp
)

target_link_libraries(tst_CompatibilityMigrationToolkit
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_CompatibilityMigrationToolkit
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_CompatibilityMigrationToolkit
    COMMAND tst_CompatibilityMigrationToolkit
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_QaMatrixRunner
    tst_QaMatrixRunner.cpp
)

target_link_libraries(tst_QaMatrixRunner
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_QaMatrixRunner
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_QaMatrixRunner
    COMMAND tst_QaMatrixRunner
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 1: Protobuf Contract Foundation ─────────────────────────────────
# Encode/decode roundtrip tests for every proto message type defined in proto/.
# Links only cme_proto (no libComponentMapEditor dependency) — these are pure
# protobuf contract tests, independent of Qt runtime.
qt_add_executable(tst_Phase1ProtoContracts
    tst_Phase1ProtoContracts.cpp
)

target_link_libraries(tst_Phase1ProtoContracts
    PRIVATE
        Qt6::Test
        cme_proto
)

add_test(
    NAME tst_Phase1ProtoContracts
    COMMAND tst_Phase1ProtoContracts
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 1: Execution Semantics v2 Compatibility ──────────────────────────
# Ensures v1 providers still work unchanged through the v2 fallback adapter and
# that simple linear graph timeline ordering remains deterministic.
qt_add_executable(tst_Phase1ExecutionSemanticsV2Compatibility
    tst_Phase1ExecutionSemanticsV2Compatibility.cpp
)

target_link_libraries(tst_Phase1ExecutionSemanticsV2Compatibility
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_Phase1ExecutionSemanticsV2Compatibility
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_Phase1ExecutionSemanticsV2Compatibility
    COMMAND tst_Phase1ExecutionSemanticsV2Compatibility
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 2: Runtime Token Routing ─────────────────────────────────────────
# Verifies feature-flagged per-edge token routing behavior in GraphExecutionSandbox
# while preserving deterministic timeline sequence.
qt_add_executable(tst_Phase2TokenRouting
    tst_Phase2TokenRouting.cpp
)

target_link_libraries(tst_Phase2TokenRouting
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_Phase2TokenRouting
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_Phase2TokenRouting
    COMMAND tst_Phase2TokenRouting
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 3: Connection Transport Metadata ─────────────────────────────────
# Verifies optional transport metadata contract roundtrip, normalization
# defaults, and deterministic validation diagnostics for malformed descriptors.
qt_add_executable(tst_Phase3ConnectionTransportMetadata
    tst_Phase3ConnectionTransportMetadata.cpp
)

target_link_libraries(tst_Phase3ConnectionTransportMetadata
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_Phase3ConnectionTransportMetadata
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
)

add_test(
    NAME tst_Phase3ConnectionTransportMetadata
    COMMAND tst_Phase3ConnectionTransportMetadata
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 4: GraphEditorController Policy Migration ────────────────────────
# Typed connection policy context implementation with backward-compatible adapters.
# Tests: typed context building, proto↔map conversions, policy decisions unchanged.
qt_add_executable(tst_Phase4PolicyMigration
    tst_Phase4PolicyMigration.cpp
)

target_link_libraries(tst_Phase4PolicyMigration
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_Phase4PolicyMigration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
)

add_test(
    NAME tst_Phase4PolicyMigration
    COMMAND tst_Phase4PolicyMigration
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 5: Execution Observability ───────────────────────────────────────
# Verifies stable debug snapshot structure, token telemetry counters, and
# payload redaction in timeline/debug snapshot outputs.
qt_add_executable(tst_Phase5ExecutionObservability
    tst_Phase5ExecutionObservability.cpp
)

target_link_libraries(tst_Phase5ExecutionObservability
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_Phase5ExecutionObservability
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_Phase5ExecutionObservability
    COMMAND tst_Phase5ExecutionObservability
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 5: ValidationService Typed Snapshot Migration ──────────────────────
# Typed graph snapshot building with validation equivalence and legacy bridging.
# Tests: typed snapshot building, severity/code enum mapping, snapshot roundtrips,
#        null graph handling, validation equivalence (typed vs legacy).
qt_add_executable(tst_Phase5ValidationMigration
    tst_Phase5ValidationMigration.cpp
)

target_link_libraries(tst_Phase5ValidationMigration
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_Phase5ValidationMigration
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
)

add_test(
    NAME tst_Phase5ValidationMigration
    COMMAND tst_Phase5ValidationMigration
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 6: GraphExecutionSandbox Typed Events ─────────────────────────────
# Internal typed event enums/messages with legacy QVariantMap timeline exposure.
# Tests: determinism, event ordering, and payload fields for start/step/pause/
#        complete/error paths.
qt_add_executable(tst_Phase6TypedExecutionEvents
    tst_Phase6TypedExecutionEvents.cpp
)

target_link_libraries(tst_Phase6TypedExecutionEvents
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_Phase6TypedExecutionEvents
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
)

add_test(
    NAME tst_Phase6TypedExecutionEvents
    COMMAND tst_Phase6TypedExecutionEvents
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 6: Cutover Readiness and Compatibility Window ───────────────────
# Verifies default-on token transport, canary enablement in example/customize
# entrypoints, and retention of v1 compatibility adapter during release window.
qt_add_executable(tst_Phase6CutoverReadiness
    tst_Phase6CutoverReadiness.cpp
)

target_link_libraries(tst_Phase6CutoverReadiness
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_Phase6CutoverReadiness
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_Phase6CutoverReadiness
    COMMAND tst_Phase6CutoverReadiness
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 7: Extension Contract V2 Parallel Support ─────────────────────────
# Mixed-mode registry/session support for V1 and V2 providers with typed core path.
qt_add_executable(tst_Phase7ExtensionContractV2Parallel
    tst_Phase7ExtensionContractV2Parallel.cpp
)

target_link_libraries(tst_Phase7ExtensionContractV2Parallel
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_Phase7ExtensionContractV2Parallel
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
)

add_test(
    NAME tst_Phase7ExtensionContractV2Parallel
    COMMAND tst_Phase7ExtensionContractV2Parallel
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 10: Cutover and Legacy Retirement ────────────────────────────────
# Deprecation + cutover checks to keep typed-first core path and monitor
# compatibility-window legacy registrations.
qt_add_executable(tst_Phase10CutoverLegacyRetirement
    tst_Phase10CutoverLegacyRetirement.cpp
)

target_link_libraries(tst_Phase10CutoverLegacyRetirement
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_Phase10CutoverLegacyRetirement
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
)

add_test(
    NAME tst_Phase10CutoverLegacyRetirement
    COMMAND tst_Phase10CutoverLegacyRetirement
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# ── Phase 11: Optional gRPC Template Descriptor Transport ──────────────────
# Verifies latency/error SLO handling and graceful fallback to local templates
# when gRPC descriptor transport is unavailable.
qt_add_executable(tst_Phase11TemplateDescriptorTransport
    tst_Phase11TemplateDescriptorTransport.cpp
)

target_link_libraries(tst_Phase11TemplateDescriptorTransport
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_Phase11TemplateDescriptorTransport
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
)

add_test(
    NAME tst_Phase11TemplateDescriptorTransport
    COMMAND tst_Phase11TemplateDescriptorTransport
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_PropertySchemaNormalizedRowBaseline
    tst_PropertySchemaNormalizedRowBaseline.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers/customizepropertyschemaprovider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers/property_schemas/connectionpropertyschemaprovider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers/property_schemas/controlpropertyschemaprovider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers/property_schemas/mathpropertyschemaprovider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers/property_schemas/startstoppropertyschemaprovider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers/property_schemas/systempropertyschemaprovider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers/property_schemas/workflowpropertyschemaprovider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/factory_pack/providers/FactoryPropertySchemaProvider.cpp
    ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/factory_pack/providers/FactoryComponentTypeProvider.cpp
)

target_link_libraries(tst_PropertySchemaNormalizedRowBaseline
    PRIVATE
        Qt6::Test
        cme_proto
        libComponentMapEditor
)

target_include_directories(tst_PropertySchemaNormalizedRowBaseline
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
        ${CMAKE_BINARY_DIR}/proto
        ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/providers
        ${CMAKE_SOURCE_DIR}/customize_example/src/extensions/factory_pack/providers
)

target_compile_definitions(tst_PropertySchemaNormalizedRowBaseline
    PRIVATE
        CME_SCHEMA_BASELINE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/baseline/property_schema_normalized_rows.json"
)

add_test(
    NAME tst_PropertySchemaNormalizedRowBaseline
    COMMAND tst_PropertySchemaNormalizedRowBaseline
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
