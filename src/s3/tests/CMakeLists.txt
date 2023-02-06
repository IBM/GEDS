#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

if(NOT HAVE_TESTS)
        message(ERROR "Trying to build tests, but testing is not active.")
endif()

add_executable(test_s3_lib
        test_S3Endpoint.cpp
)
target_link_libraries(test_s3_lib
        PUBLIC
        gtest_main
        geds_s3
        ${AWSSDK_LIBRARIES}
)

if (DEFINED ENV{CI})
        message(WARNING "S3 Tests disabled.")
        gtest_discover_tests(test_s3_lib)
endif()

set(S3_TEST_ENDPOINT $ENV{S3_TEST_ENDPOINT} "http://localhost")
set(S3_TEST_ACCESS_KEY $ENV{S3_TEST_ACCESS_KEY} "test")
set(S3_TEST_SECRET_KEY $ENV{S3_TEST_SECRET_KEY} "test")

target_compile_definitions(test_s3_lib PRIVATE
        -DS3_TEST_ENDPOINT=S3_TEST_ENDPOINT
        -DS3_TEST_ACCESS_KEY=S3_TEST_ACCESS_KEY
        -DS3_TEST_SECRET_KEY=S3_TEST_ACCESS_KEY
)

install(TARGETS test_s3_lib
        COMPONENT geds)
