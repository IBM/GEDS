#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

protobuf_generate_grpc_cpp(helloworld.proto)

set(HELLOWORLD_PROTOBUF_SOURCES
        helloworld.proto
        "${_gRPC_PROTO_GENS_DIR}/helloworld.grpc.pb.cc"
        "${_gRPC_PROTO_GENS_DIR}/helloworld.grpc.pb.h"
        "${_gRPC_PROTO_GENS_DIR}/helloworld_mock.grpc.pb.h"
        "${_gRPC_PROTO_GENS_DIR}/helloworld.pb.cc"
        "${_gRPC_PROTO_GENS_DIR}/helloworld.pb.h"
        )

add_executable(helloworld_client client.cpp ${HELLOWORLD_PROTOBUF_SOURCES})
target_link_libraries(helloworld_client
        PUBLIC
        protobuf::libprotobuf
        ${GRPC_LIBRARIES})

        target_compile_options(helloworld_client PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

target_include_directories(helloworld_client PRIVATE "${_gRPC_PROTO_GENS_DIR}")

add_executable(helloworld_server server.cpp ${HELLOWORLD_PROTOBUF_SOURCES})
target_link_libraries(helloworld_server
        PUBLIC
        protobuf::libprotobuf
        ${GRPC_LIBRARIES})

target_compile_options(helloworld_server PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})


target_include_directories(helloworld_server PRIVATE "${_gRPC_PROTO_GENS_DIR}")
