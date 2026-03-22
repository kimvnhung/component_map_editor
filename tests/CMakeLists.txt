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
)

# QuickTest's default discovery scans the current working directory for
# tst_*.qml files. Copy the file into build/tests so direct IDE launches and
# ctest both work without requiring source-dir arguments.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/tst_InteractionStateManager.qml
    ${CMAKE_CURRENT_BINARY_DIR}/tst_InteractionStateManager.qml
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

qt_add_executable(tst_GraphModelIndexing
    tst_GraphModelIndexing.cpp
)

target_link_libraries(tst_GraphModelIndexing
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_GraphModelIndexing
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_GraphModelIndexing
    COMMAND tst_GraphModelIndexing
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

qt_add_executable(tst_GraphExecutionSandbox
    tst_GraphExecutionSandbox.cpp
)

target_link_libraries(tst_GraphExecutionSandbox
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_GraphExecutionSandbox
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_GraphExecutionSandbox
    COMMAND tst_GraphExecutionSandbox
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

qt_add_executable(tst_SecurityGuardrails
    tst_SecurityGuardrails.cpp
)

target_link_libraries(tst_SecurityGuardrails
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_SecurityGuardrails
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_SecurityGuardrails
    COMMAND tst_SecurityGuardrails
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

qt_add_executable(tst_PerformanceScaleHardening
    tst_PerformanceScaleHardening.cpp
)

target_link_libraries(tst_PerformanceScaleHardening
    PRIVATE
        Qt6::Test
        libComponentMapEditor
)

target_include_directories(tst_PerformanceScaleHardening
    PRIVATE
        ${CMAKE_SOURCE_DIR}/component_map_editor
)

add_test(
    NAME tst_PerformanceScaleHardening
    COMMAND tst_PerformanceScaleHardening
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
