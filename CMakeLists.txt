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

add_subdirectory(component_map_editor)
add_subdirectory(example)
