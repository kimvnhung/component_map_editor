cmake_minimum_required(VERSION 3.21)

project(component_map_runtime VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core)

set(COMPONENT_MAP_RUNTIME_HEADERS
    include/component_map_runtime/RuntimeService.h
    include/component_map_runtime/RuntimeTypes.h
    include/component_map_runtime/RuntimeHandlerRegistry.h
    include/component_map_runtime/DeploymentValidator.h
    include/component_map_runtime/DeploymentManager.h
    include/component_map_runtime/RuntimeCore.h
    include/component_map_runtime/RuntimePersistence.h
    include/component_map_runtime/RuntimeGrpcServer.h
)

set(COMPONENT_MAP_RUNTIME_SOURCES
    src/RuntimeService.cpp
    src/RuntimeHandlerRegistry.cpp
    src/DeploymentValidator.cpp
    src/DeploymentManager.cpp
    src/RuntimeCore.cpp
    src/RuntimePersistence.cpp
    src/RuntimeGrpcServer.cpp
)

add_library(component_map_runtime STATIC
    ${COMPONENT_MAP_RUNTIME_HEADERS}
    ${COMPONENT_MAP_RUNTIME_SOURCES}
)

target_link_libraries(component_map_runtime
    PUBLIC
        Qt6::Core
        cme_proto
        base_log
        SQLite::SQLite3
        cme_runtime_proto
        gRPC::grpc++
)

target_include_directories(component_map_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(component_map_runtime PUBLIC cxx_std_17)
target_compile_definitions(component_map_runtime PRIVATE LOG_TAG="ComponentMapRuntime")

if(CME_BUILD_RUNTIME)
    qt_add_executable(component_map_runtime_process
        main.cpp
    )

    target_link_libraries(component_map_runtime_process
        PRIVATE
            Qt6::Core
            component_map_runtime
    )

    target_compile_definitions(component_map_runtime_process
        PRIVATE
            LOG_TAG="ComponentMapRuntimeProcess"
    )
endif()
