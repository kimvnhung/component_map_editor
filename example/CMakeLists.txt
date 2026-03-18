find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2)

qt_add_executable(example_app
    main.cpp
)

set(EXAMPLE_EXTENSION_MANIFEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/extensions")
file(MAKE_DIRECTORY "${EXAMPLE_EXTENSION_MANIFEST_DIR}")
configure_file(
    "${CMAKE_SOURCE_DIR}/component_map_editor/extensions/sample_pack/manifest.sample.workflow.json"
    "${EXAMPLE_EXTENSION_MANIFEST_DIR}/manifest.sample.workflow.json"
    COPYONLY
)

qt_add_qml_module(example_app
    URI ExampleApp
    VERSION 1.0
    QML_FILES
        Main.qml
        BenchmarkPanel.qml
    SOURCES
        BenchmarkHelper.h
        BenchmarkHelper.cpp
        FrameSwapTelemetry.h
        FrameSwapTelemetry.cpp
)

# Link against the ComponentMapEditor backing library and its generated QML plugin.
target_link_libraries(example_app
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        libComponentMapEditor
)

target_include_directories(example_app
    PRIVATE
        "${CMAKE_SOURCE_DIR}/component_map_editor"
)

target_compile_definitions(example_app
    PRIVATE
        EXAMPLE_EXTENSION_MANIFEST_DIR="${EXAMPLE_EXTENSION_MANIFEST_DIR}"
)
