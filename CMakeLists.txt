cmake_minimum_required(VERSION 3.21)

project(component_map_editor VERSION 1.0.0 LANGUAGES CXX)

# Set C++17 standard and auto moc
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2)

qt_standard_project_setup(REQUIRES 6.5)

option(CME_BUILD_EXAMPLES "Build example applications" OFF)
option(CME_BUILD_TESTS "Build test targets" OFF)

# ── Protobuf ────────────────────────────────────────────────────────────────
# Prefer the system protoc that matches libprotobuf-dev (3.21.12).
# /usr/local/bin/protoc ships an older 3.6.1 build; pin to /usr/bin/protoc.
find_program(Protobuf_PROTOC_EXECUTABLE
    NAMES protoc
    PATHS /usr/bin
    NO_DEFAULT_PATH
    DOC "protoc compiler matched to libprotobuf-dev 3.21.12"
)
find_package(Protobuf REQUIRED)
message(STATUS "Protobuf version : ${Protobuf_VERSION}")
message(STATUS "protoc           : ${Protobuf_PROTOC_EXECUTABLE}")
# ────────────────────────────────────────────────────────────────────────────

add_subdirectory(qml_fontawesome)
add_subdirectory(proto)
add_subdirectory(component_map_editor)

if(CME_BUILD_EXAMPLES)
    add_subdirectory(example)
    add_subdirectory(customize_example)
endif()

if(CME_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

