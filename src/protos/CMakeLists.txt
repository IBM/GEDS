#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

file(MAKE_DIRECTORY "${_gRPC_PROTO_GENS_DIR}")

protobuf_generate_grpc_cpp(status.proto)
protobuf_generate_grpc_cpp(geds.proto)

add_library(geds_proto STATIC
        ObjectStoreConfig.h
        Object.h
        ParseGRPC.cpp
        ParseGRPC.h
        Status.cpp
        Status.h
        geds.proto
        status.proto
        "${_gRPC_PROTO_GENS_DIR}/geds.grpc.pb.cc"
        "${_gRPC_PROTO_GENS_DIR}/geds.grpc.pb.h"
        "${_gRPC_PROTO_GENS_DIR}/geds.pb.cc"
        "${_gRPC_PROTO_GENS_DIR}/geds.pb.h"
        "${_gRPC_PROTO_GENS_DIR}/geds_mock.grpc.pb.h"
        "${_gRPC_PROTO_GENS_DIR}/status.grpc.pb.cc"
        "${_gRPC_PROTO_GENS_DIR}/status.grpc.pb.h"
        "${_gRPC_PROTO_GENS_DIR}/status.pb.cc"
        "${_gRPC_PROTO_GENS_DIR}/status.pb.h"
        "${_gRPC_PROTO_GENS_DIR}/status_mock.grpc.pb.h"
        PubSub.h)
target_link_libraries(geds_proto
        PUBLIC
        ${Protobuf_LIBRARIES}
        ${GRPC_LIBRARIES}
        ${GEDS_EXTRA_LINKER_FLAGS}
)
set_target_properties(geds_proto
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)
target_compile_options(geds_proto PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
target_include_directories(geds_proto
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_gRPC_PROTO_GENS_DIR}")

if(HAVE_TESTS)
        add_subdirectory(tests)
endif()
