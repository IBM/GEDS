#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

if (NOT HAVE_RUST_BINDINGS)
    message(error "This module requires the Rust bindings flag.")
else ()
    message(STATUS "Installing Rust library.")
endif ()

# Install Rust library files
install(FILES build.rs Cargo.toml Cargo.lock
    COMPONENT geds
    DESTINATION rust/
    COMPONENT geds)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/rust/src" DESTINATION rust/ COMPONENT geds)

# GEDS libgeds headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/libgeds/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
)

# GEDS utility headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/utility/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
)

# GEDS common headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/common/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
)

# GEDS protos headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/protos/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
)

# GEDS s3 headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/s3/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
)

# GEDS statistics headers
install(DIRECTORY "${CMAKE_SOURCE_DIR}/src/statistics/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
)

# GEDS gens headers
install(DIRECTORY "${CMAKE_BINARY_DIR}/gens/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.inc"
)

# grpc headers
install(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/grpc-src/include/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.inc"
)

# abseil headers
install(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/grpc-src/third_party/abseil-cpp/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.inc"
)

# protobuf headers
install(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/grpc-src/third_party/protobuf/src/"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.inc"
)

# boost headers
install(DIRECTORY "${BOOST_ROOT}/include/boost"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.ipp"
    PATTERN "*.h"
)

# AWS S3 SDK headers
install(DIRECTORY "${AWSSDK_ROOT}/include"
    COMPONENT geds
    DESTINATION "rust/include"
    FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.ipp"
    PATTERN "*.h"
    PATTERN "*.inc"
    PATTERN "*.inl"
)
