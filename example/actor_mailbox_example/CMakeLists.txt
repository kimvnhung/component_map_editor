cmake_minimum_required(VERSION 3.10)
project(actor_mailbox_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#find qt core
find_package(Qt6 COMPONENTS Core Gui Concurrent Quick REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)
if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    *.cpp
    *.h
)

# Remove main.cpp from SOURCE_FILES
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

file(GLOB_RECURSE QML_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.qml
)

qt_add_executable(${PROJECT_NAME}
    main.cpp
)

qt_add_qml_module(${PROJECT_NAME}
    URI ActorMailboxExample
    VERSION 1.0
    QML_FILES
        ${QML_FILES}
    SOURCES
        ${SOURCE_FILES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        base_log
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Concurrent
)