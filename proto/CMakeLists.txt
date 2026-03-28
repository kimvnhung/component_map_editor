# proto/CMakeLists.txt
# Generates C++ protobuf code from the 5 .proto files and packages them into
# a static library (cme_proto) that other targets can link against.

cmake_minimum_required(VERSION 3.21)

set(PROTO_FILES
    command.proto
    graph.proto
    validation.proto
    execution.proto
    policy.proto
)

# generate *.pb.cc / *.pb.h pairs into CMAKE_CURRENT_BINARY_DIR
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

add_library(cme_proto STATIC
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# Suppress common warnings in Google-generated code
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(cme_proto PRIVATE -w)
endif()

target_link_libraries(cme_proto
    PUBLIC
        protobuf::libprotobuf
)

# Consumers only need this binary dir on the include path; the protobuf
# headers come through the transitive link dependency.
target_include_directories(cme_proto
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
)
