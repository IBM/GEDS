#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

SET(SOURCES
    Statistics.cpp
    Statistics.h
    StatisticsCounter.cpp
    StatisticsCounter.h
    StatisticsGauge.cpp
    StatisticsGauge.h
    StatisticsHistogram.h
    StatisticsItem.cpp
    StatisticsItem.h
)

add_library(geds_statistics STATIC ${SOURCES})
target_link_libraries(geds_statistics
    PUBLIC
    ${Boost_LIBRARIES}
    geds_utility
    ${GEDS_EXTRA_LINKER_FLAGS}
)
target_include_directories(geds_statistics
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_compile_options(geds_statistics PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
set_target_properties(geds_statistics
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
