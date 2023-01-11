#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

if (NOT HAVE_TESTS)
        message(ERROR "Trying to build tests, but testing is not active.")
endif()

add_executable(test_protos_lib
    test_parse_grpc.cpp
)
target_link_libraries(test_protos_lib
        PUBLIC
                gtest_main
                geds_proto
)
gtest_discover_tests(test_protos_lib)

install(TARGETS test_protos_lib
        COMPONENT geds)
