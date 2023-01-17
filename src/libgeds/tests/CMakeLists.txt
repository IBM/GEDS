#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

if(NOT HAVE_TESTS)
        message(ERROR "Trying to build tests, but testing is not active.")
endif()

add_executable(test_geds_lib
        test_Filesystem.cpp
        test_GEDS.cpp
        test_GEDSFile.cpp
        test_GEDSFileHandle.cpp
        test_GEDSS3FileHandle.cpp
)
target_link_libraries(test_geds_lib
        PUBLIC
        gtest_main
        libgeds
)
target_compile_options(test_geds_lib PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

gtest_discover_tests(test_geds_lib)

install(TARGETS test_geds_lib
        COMPONENT geds)
