#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

if(NOT HAVE_TESTS)
        message(ERROR "Trying to build tests, but testing is not active.")
endif()

add_executable(test_utility
        test_Path.cpp
)
target_link_libraries(test_utility
        PUBLIC
        gtest_main
        geds_utility
)
target_compile_options(test_utility PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

gtest_discover_tests(test_utility)

install(TARGETS test_utility
        COMPONENT geds)
