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
find_package(protobuf CONFIG REQUIRED)

if(TARGET protobuf::protoc)
    message(STATUS "Found protobuf::protoc")
else()
    message(FATAL_ERROR "protobuf::protoc not found")
endif()
get_target_property(
    Protobuf_PROTOC_EXECUTABLE
    protobuf::protoc
    IMPORTED_LOCATION
)
if(NOT Protobuf_PROTOC_EXECUTABLE)
    message(FATAL_ERROR "Could not find protoc executable")
else()
    message(STATUS "Found protoc executable: ${Protobuf_PROTOC_EXECUTABLE}")
endif()
# ────────────────────────────────────────────────────────────────────────────

add_subdirectory(qml_fontawesome)
add_subdirectory(proto)
add_subdirectory(base_log)
add_subdirectory(component_map_editor)

if(CME_BUILD_EXAMPLES)
    add_subdirectory(example)
    add_subdirectory(customize_example)
endif()

if(CME_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

