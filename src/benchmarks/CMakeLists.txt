#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

# IO Benchmark
add_executable(benchmark_io benchmark_io.cpp)
target_link_libraries(benchmark_io
    PRIVATE
    absl::flags
    absl::flags_parse
    libgeds)
target_compile_options(benchmark_io PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

# Shuffle Serve Benchmark
add_executable(shuffle_serve shuffle_serve.cpp)
target_link_libraries(shuffle_serve
    PRIVATE
    absl::flags
    absl::flags_parse
    libgeds)
target_compile_options(shuffle_serve PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

# Shuffle Read Benchmark
add_executable(shuffle_read shuffle_read.cpp)
target_link_libraries(shuffle_read
    PRIVATE
    absl::flags
    absl::flags_parse
    libgeds)
target_compile_options(shuffle_read PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

# Install all targets
install(TARGETS
    benchmark_io
    shuffle_serve
    shuffle_read
    COMPONENT geds)
