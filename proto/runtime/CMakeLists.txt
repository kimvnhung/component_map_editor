set(RUNTIME_PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/control.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/event.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/query.proto
)

set(RUNTIME_PROTO_SRCS)
set(RUNTIME_PROTO_HDRS)
set(RUNTIME_GRPC_SRCS)
set(RUNTIME_GRPC_HDRS)

foreach(PROTO_FILE IN LISTS RUNTIME_PROTO_FILES)
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    list(APPEND RUNTIME_PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc)
    list(APPEND RUNTIME_PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h)
    list(APPEND RUNTIME_GRPC_SRCS ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc)
    list(APPEND RUNTIME_GRPC_HDRS ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.cc
               ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb.h
               ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.cc
               ${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.grpc.pb.h
        COMMAND protobuf::protoc
        ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
             --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
             --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin
             -I ${CMAKE_CURRENT_SOURCE_DIR}
             -I ${CMAKE_SOURCE_DIR}/proto
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE} protobuf::protoc
        VERBATIM
    )
endforeach()

add_library(cme_runtime_proto STATIC
    ${RUNTIME_PROTO_SRCS}
    ${RUNTIME_PROTO_HDRS}
    ${RUNTIME_GRPC_SRCS}
    ${RUNTIME_GRPC_HDRS}
)

target_include_directories(cme_runtime_proto PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/proto
)
target_link_libraries(cme_runtime_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)
set_target_properties(cme_runtime_proto PROPERTIES POSITION_INDEPENDENT_CODE ON)
