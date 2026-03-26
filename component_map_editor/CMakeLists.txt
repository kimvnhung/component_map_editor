cmake_minimum_required(VERSION 3.14)

project(libComponentMapEditor VERSION 1.0.0 LANGUAGES CXX)
qt_policy(SET QTP0004 NEW)
find_package(Qt6 REQUIRED COMPONENTS Core Quick)

# set headers
file(GLOB_RECURSE COMPONENT_MAP_EDITOR_HEADERS CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.h
)

# set sources
file(GLOB_RECURSE COMPONENT_MAP_EDITOR_SOURCES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.cpp
)

file(GLOB_RECURSE COMPONENT_MAP_EDITOR_QML RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ui/*.qml
)

file(GLOB_RECURSE COMPONENT_MAP_EDITOR_JS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ui/*.js
)

qt_add_qml_module(${PROJECT_NAME}
    URI ComponentMapEditor
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ComponentMapEditor"
    IMPORTS
        QmlFontAwesome
    QML_FILES
        ${COMPONENT_MAP_EDITOR_QML}
        ${COMPONENT_MAP_EDITOR_JS}
    SOURCES
        ${COMPONENT_MAP_EDITOR_HEADERS}
        ${COMPONENT_MAP_EDITOR_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt6::Core
        Qt6::Quick
    QmlFontAwesome::QmlFontAwesome
)

# Copy HEADER_FILES to folder include in build directory
foreach(HEADER_FILE ${COMPONENT_MAP_EDITOR_HEADERS})
    get_filename_component(HEADER_FILE_NAME ${HEADER_FILE} NAME)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_FILE}
        ${CMAKE_CURRENT_BINARY_DIR}/include/${HEADER_FILE_NAME}
        COPYONLY)
endforeach()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)
