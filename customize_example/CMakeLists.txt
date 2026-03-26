cmake_minimum_required(VERSION 3.16)

project(customize_example VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

qt_add_executable(customize_example_app
    main.cpp
)

set(EXAMPLE_EXTENSION_MANIFEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/extensions")
file(MAKE_DIRECTORY "${EXAMPLE_EXTENSION_MANIFEST_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/extensions/manifests/manifest.customize.workflow.json"
    "${EXAMPLE_EXTENSION_MANIFEST_DIR}/manifest.customize.workflow.json"
    COPYONLY
)

set(EXAMPLE_EXTENSION_RULE_DIR "${CMAKE_CURRENT_BINARY_DIR}/rules")
file(MAKE_DIRECTORY "${EXAMPLE_EXTENSION_RULE_DIR}")
set(EXAMPLE_EXTENSION_RULE_FILE "${EXAMPLE_EXTENSION_RULE_DIR}/rules.customize.workflow.json")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/extensions/manifests/rules.customize.workflow.json"
    "${EXAMPLE_EXTENSION_RULE_FILE}"
    COPYONLY
)

file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.qml
)

qt_add_qml_module(customize_example_app
    URI customize_example
    VERSION 1.0
    QML_FILES
        ${QML_FILES}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(customize_example_app PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.customize_example_app
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(customize_example_app
    PRIVATE
        Qt6::Core
        Qt6::Quick
        libComponentMapEditor
)


target_compile_definitions(example_app
    PRIVATE
        EXAMPLE_EXTENSION_MANIFEST_DIR="${EXAMPLE_EXTENSION_MANIFEST_DIR}"
        EXAMPLE_EXTENSION_RULE_FILE="${EXAMPLE_EXTENSION_RULE_FILE}"
)

install(TARGETS customize_example_app
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
