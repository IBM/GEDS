#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

SET(SOURCES
    Statistics.h
    Statistics.cpp
    StatisticsCounter.h
    StatisticsCounter.cpp
)

add_library(geds_statistics STATIC ${SOURCES})
target_link_libraries(geds_statistics
    PUBLIC
    geds_utility
)
target_include_directories(geds_statistics
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_options(geds_statistics PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
set_target_properties(geds_statistics
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
