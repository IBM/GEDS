#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set(SOURCES
        GRPCServer.cpp
        GRPCServer.h
        ObjectStoreHandler.cpp
        ObjectStoreHandler.h
        S3Helper.cpp
        S3Helper.h
        PubSubMQTT.h        
)

add_library(libmetadataservice STATIC ${SOURCES})
target_link_libraries(libmetadataservice
        PUBLIC
        ${GRPC_LIBRARIES}
        geds_utility
        geds_proto
        geds_s3
        paho-mqttpp3
)
target_compile_options(libmetadataservice PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
target_compile_definitions(libmetadataservice
        PUBLIC
        HAVE_DEFAULT_BUCKET=$<BOOL:${HAVE_DEFAULT_BUCKET}>
        DEFAULT_BUCKET_NAME=${DEFAULT_BUCKET_NAME}
)
set_target_properties(libmetadataservice
        PROPERTIES
        OUTPUT_NAME "metadataservice")

add_executable(metadataserver main.cpp)
target_link_libraries(metadataserver
        PUBLIC
        absl::flags
        absl::flags_parse
        geds_proto
        libmetadataservice
)

target_compile_options(metadataserver PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

install(TARGETS metadataserver
        COMPONENT geds)

if(HAVE_TESTS)
        add_executable(test_metadataserver test_KVS.cpp)
        target_link_libraries(test_metadataserver
                PUBLIC
                libmetadataservice
                gtest_main
        )
        gtest_discover_tests(test_metadataserver)

        install(TARGETS test_metadataserver
                COMPONENT geds)
endif()
