cmake_minimum_required(VERSION 3.21)

project(QmlFontAwesome VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

set(QML_FONT_AWESOME_SOURCES
    src/FontAwesome.h
    src/FontAwesome.cpp
)

set(QML_FONT_AWESOME_QML_FILES
    qml/FaIcon.qml
)

set(QML_FONT_AWESOME_RESOURCES
    fontawesome/LICENSE.txt
    fontawesome/metadata/icons.json
    "fontawesome/otfs/Font Awesome 7 Brands-Regular-400.otf"
    "fontawesome/otfs/Font Awesome 7 Free-Regular-400.otf"
    "fontawesome/otfs/Font Awesome 7 Free-Solid-900.otf"
)

qt_add_qml_module(QmlFontAwesome
    URI QmlFontAwesome
    VERSION 1.0
    SOURCES
        ${QML_FONT_AWESOME_SOURCES}
    QML_FILES
        ${QML_FONT_AWESOME_QML_FILES}
    RESOURCES
        ${QML_FONT_AWESOME_RESOURCES}
)

target_link_libraries(QmlFontAwesome
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

target_include_directories(QmlFontAwesome
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/QmlFontAwesome>
)

set_target_properties(QmlFontAwesome PROPERTIES
    EXPORT_NAME QmlFontAwesome
)

add_library(QmlFontAwesome::QmlFontAwesome ALIAS QmlFontAwesome)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS QmlFontAwesome
    EXPORT QmlFontAwesomeTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
    src/FontAwesome.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/QmlFontAwesome
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/QmlFontAwesomeConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/QmlFontAwesomeConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QmlFontAwesome
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/QmlFontAwesomeConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/QmlFontAwesomeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/QmlFontAwesomeConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QmlFontAwesome
)

install(EXPORT QmlFontAwesomeTargets
    FILE QmlFontAwesomeTargets.cmake
    NAMESPACE QmlFontAwesome::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/QmlFontAwesome
)
