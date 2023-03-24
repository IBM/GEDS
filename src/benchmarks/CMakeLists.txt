#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

add_executable(shuffle_serve shuffle_serve.cpp)
target_link_libraries(shuffle_serve
    PRIVATE
    absl::flags
    absl::flags_parse
    libgeds)
target_compile_options(shuffle_serve PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

install(TARGETS shuffle_serve
    COMPONENT geds)

add_executable(shuffle_read shuffle_read.cpp)
target_link_libraries(shuffle_read
    PRIVATE
    absl::flags
    absl::flags_parse
    libgeds)
target_compile_options(shuffle_read PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

install(TARGETS shuffle_read
    COMPONENT geds)
