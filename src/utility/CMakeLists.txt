#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/Version.cpp" @ONLY)

SET(SOURCES
        ByteStream.h
        ConcurrentMap.h
        ConcurrentSet.h
        FormatISO8601.h
        Logging.h
        Path.h
        Path.cpp
        Platform.h
        Platform.cpp
        Ports.h
        Version.h
        ${CMAKE_CURRENT_BINARY_DIR}/Version.cpp
)

add_library(geds_utility STATIC ${SOURCES})
target_include_directories(geds_utility
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${BOOST_INCLUDE_DIRS}
)
target_compile_options(geds_utility PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
set_target_properties(geds_utility
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

if(HAVE_TESTS)
        add_subdirectory(tests)
endif()
