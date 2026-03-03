cmake_minimum_required(VERSION 3.14)

project(libComponentMapEditor VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Quick)

file(GLOB_RECURSE COMPONENT_MAP_EDITOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

file(GLOB_RECURSE COMPONENT_MAP_EDITOR_QML RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ui/*.qml
)

qt_add_qml_module(${PROJECT_NAME}
    URI ComponentMapEditor
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ComponentMapEditor"
    QML_FILES
        ${COMPONENT_MAP_EDITOR_QML}
    SOURCES
        ${COMPONENT_MAP_EDITOR_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt6::Core
        Qt6::Quick
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)