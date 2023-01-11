#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

set(SOURCES
        main.cpp)

add_executable(benchmark ${SOURCES})
target_link_libraries(benchmark
        PRIVATE
        absl::flags
        absl::flags_parse
        libgeds)
target_compile_options(benchmark PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

install(TARGETS benchmark
        COMPONENT geds)
