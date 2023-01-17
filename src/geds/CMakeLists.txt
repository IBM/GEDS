#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set(SOURCES
        main.cpp)

add_executable(geds ${SOURCES})
target_link_libraries(geds
        PRIVATE
        absl::flags
        absl::flags_parse
        libgeds)
target_compile_options(geds PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

install(TARGETS geds
        COMPONENT geds)
