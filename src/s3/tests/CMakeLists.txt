#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

if (NOT HAVE_TESTS)
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
gtest_discover_tests(test_s3_lib)

install(TARGETS test_s3_lib
        COMPONENT geds)
