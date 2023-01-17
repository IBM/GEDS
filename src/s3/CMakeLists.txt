#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

SET(SOURCES
    S3Endpoint.cpp
    S3Endpoint.h
    S3Init.cpp
    S3Init.h
    S3ObjectStores.cpp
    S3ObjectStores.h
)

add_library(geds_s3 STATIC ${SOURCES})
target_link_libraries(geds_s3
    PUBLIC
    ${GRPC_LIBRARIES}
    geds_common
    geds_utility
    geds_proto
    geds_statistics
    ${AWSSDK_LINK_LIBRARIES}
    ${AWSSDK_PLATFORM_DEPS}
)

target_include_directories(geds_s3
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AWSSDK_INCLUDE_DIRS}
)
set_target_properties(geds_s3
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if(HAVE_TESTS)
    add_subdirectory(tests)
endif()
