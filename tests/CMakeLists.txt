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
