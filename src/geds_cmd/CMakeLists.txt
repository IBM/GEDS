#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

set(SOURCES
        main.cpp)

add_executable(geds_cmd ${SOURCES})
target_link_libraries(geds_cmd
        PUBLIC
        absl::flags
        absl::flags_parse
        libgeds
        libgeds
)
target_compile_options(geds_cmd PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

install(TARGETS geds_cmd
        COMPONENT geds)
