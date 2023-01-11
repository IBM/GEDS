#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#


set(SOURCES
    DirectoryMarker.cpp
    DirectoryMarker.h
    GEDSFileStatus.h
)

add_library(geds_common STATIC ${SOURCES})

target_compile_options(geds_common PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
target_compile_definitions(geds_common
        PUBLIC
        "HAVE_RDMA=$<BOOL:${HAVE_RDMA}>"
        _POSIX_C_SOURCE=200809L
)
target_include_directories(geds_common
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(geds_common
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)
