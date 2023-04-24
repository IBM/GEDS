#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.22)
project(GEDS
    DESCRIPTION "GEDS"
    LANGUAGES CXX)

option(USE_EXTERNAL_GRPC "Use external GRPC (set GRPC_INSTALL_DIR env variable accordingly).")

option(HAVE_TESTS "Enable tests" ON)
option(HAVE_PYTHON_BINDINGS "Enable python bindings" ON)
option(HAVE_JAVA_BINDINGS "Enable Java bindings (SET JAVA_HOME env variable accordingly)." ON)
option(HAVE_RDMA_SUPPORT "Supports RDMA" OFF)
option(HAVE_DEFAULT_BUCKET "Creates a default (default) bucket." ON)
option(HAVE_PROMETHEUS_HISTOGRAM_BUCKETS "Enable Prometheus Histogram Buckets." OFF)

option(ENABLE_PROFILING "Compile with -pg to enable gprof." OFF)

set(DEFAULT_BUCKET_NAME "default" CACHE STRING "Name of the default bucket.")
set(BOOST_VERSION "1.80.0" CACHE STRING "Boost version")

message(STATUS "HAVE_TESTS:           ${HAVE_TESTS}")
message(STATUS "HAVE_PYTHON_BINDINGS: ${HAVE_PYTHON_BINDINGS}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(HAVE_PROMETHEUS_HISTOGRAM_BUCKETS)
    add_compile_definitions(HAVE_PROMETHEUS_HISTOGRAM_BUCKETS=1)
endif()

if (ENABLE_PROFILING)
    message(STATUS "ENABLE_PROFILING:     ${ENABLE_PROFILING}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(compilerIsGNU $<CMAKE_CXX_COMPILER_ID:GNU>)
set(compilerIsClang $<CMAKE_CXX_COMPILER_ID:GNU>)

set(GEDS_EXTRA_COMPILER_FLAGS -Wall -Wextra -Werror) # -Wpedantic # error: ISO C++ does not support ‘__int128’ for ‘type name’ [-Werror=pedantic]
set(GEDS_EXTRA_LINKER_FLAGS  )
if(${compilerIsClang})
    set(GEDS_EXTRA_COMPILER_FLAGS ${GEDS_EXTRA_COMPILER_FLAGS} -Wpedantic)
endif()

# #####################
# ## Dependencies
if(APPLE)
    # support mkdtemp.
    add_compile_options(-D_DARWIN_C_SOURCE)
endif()

include(FetchContent)

# # GRPC
set(ABSL_CXX_STANDARD ${CMAKE_CXX_STANDARD})
set(ABSL_PROPAGATE_CXX_STD ON)

set(gRPC_SSL_PROVIDER package)

if(USE_EXTERNAL_GRPC)
    if(DEFINED ENV{GRPC_INSTALL_DIR})
        set(GRPC_INSTALL_DIR $ENV{GRPC_INSTALL_DIR})
    else()
        set(GRPC_INSTALL_DIR "/usr/local")
    endif()

    find_package(absl REQUIRED CONFIG
        HINTS ${GRPC_INSTALL_DIR})
    find_package(Protobuf REQUIRED CONFIG
        HINTS ${GRPC_INSTALL_DIR})
    find_package(gRPC REQUIRED CONFIG
        HINTS ${GRPC_INSTALL_DIR})

    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin
        HINTS ${GRPC_INSTALL_DIR}/bin/)
    find_program(_gRPC_PROTOBUF_PROTOC_EXECUTABLE protoc
        HINTS ${GRPC_INSTALL_DIR}/bin)
    include("cmake/grpc-helpers.cmake")

    set(GRPC_LIBRARIES
        gRPC::grpc
        gRPC::grpc++
        gRPC::grpc++_reflection
    )
else()
    message(STATUS "Using gRPC via add_subdirectory (FetchContent).")

    FetchContent_Declare(
        gRPC
        GIT_REPOSITORY https://github.com/grpc/grpc
        GIT_TAG v1.51.1
    )

    # Disable unused plugins
    set(gRPC_BUILD_GRPC_CSHARP_PLUGIN OFF)
    set(gRPC_BUILD_GRPC_NODE_PLUGIN OFF)
    set(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN OFF)
    set(gRPC_BUILD_GRPC_PHP_PLUGIN OFF)
    set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF)
    set(gRPC_BUILD_GRPC_RUBY_PLUGIN OFF)
    set(gRPC_BUILD_TESTS OFF)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_MakeAvailable(gRPC)

    # Configure metadata in order for the plugin to work.
    set(_PROTOBUF_LIBPROTOBUF libprotobuf)
    set(_REFLECTION grpc++_reflection)
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
    set(_GRPC_GRPCPP grpc++)
    set(_gRPC_PROTO_SRCS_DIR "${CMAKE_BINARY_DIR}/protos")
    set(_gRPC_PROTO_GENS_DIR "${CMAKE_BINARY_DIR}/gens")
    set(_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR "${PROTOBUF_ROOT_DIR}/src")

    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
    set(_gRPC_PROTOBUF_PROTOC_EXECUTABLE ${_PROTOBUF_PROTOC})

    set(GRPC_LIBRARIES
        grpc
        grpc++
        grpc++_reflection
    )
endif()

set(_gRPC_PROTO_GENS_DIR "${CMAKE_BINARY_DIR}/gens")
set(_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR "${PROTOBUF_ROOT_DIR}/src")

# # Magic Enum
# https://github.com/Neargye/magic_enum (MIT)
FetchContent_Declare(
    magic_enum
    URL https://github.com/Neargye/magic_enum/archive/3d1f6a5a2a3fbcba077e00ad0ccc2dd9fefc2ca7.zip # v0.7.3
)
FetchContent_MakeAvailable(magic_enum)

# S3
if(DEFINED ENV{AWS_SDK_INSTALL_DIR})
    set(AWSSDK_ROOT $ENV{AWS_SDK_INSTALL_DIR})
elseif(DEFINED ENV{AWSSDK_ROOT})
    set(AWSSDK_ROOT $ENV{AWSSDK_ROOT})
else()
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(AWSSDK_ROOT "/usr/local/opt/aws-sdk-cpp${CMAKE_CXX_STANDARD}")
    else()
        set(AWSSDK_ROOT "/usr/local/opt/aws-sdk-cpp${CMAKE_CXX_STANDARD}_${CMAKE_BUILD_TYPE}")
    endif()
endif()

message(STATUS "Using ${AWSSDK_ROOT} to search for AWS.")

find_package(AWSSDK REQUIRED COMPONENTS s3)
message(STATUS "AWS SDK Libraries:  ${AWSSDK_LINK_LIBRARIES}")

# Boost
if(DEFINED ENV{BOOST_INSTALL_DIR})
    set(BOOST_ROOT $ENV{BOOST_INSTALL_DIR})
elseif(DEFINED ENV{BOOST_ROOT})
    set(BOOST_ROOT $ENV{BOOST_ROOT})
else()
    set(BOOST_ROOT "/usr/local/opt/boost-${BOOST_VERSION}")
endif()

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost ${BOOST_VERSION}
    REQUIRED
    system
)

# # Python bindings
if(HAVE_PYTHON_BINDINGS)
    # LTO workaround for GCC <= 8 and Clang <= 10.
    set(compilerRequiresPybindLTOworkaroundGCC $<$<AND:$<compilerIsGNU>,$<VERSION_LESS:$<CMAKE_CXX_COMILER_VERSION,9>>>)
    set(compilerRequiresPybindLTOworkaroundClang $<$<AND:$<compilerIsClang>,$<VERSION_LESS:$<CMAKE_CXX_COMILER_VERSION,11>>>)
    set(compilerRequiresPybindLTOworkaround $<OR:$<compilerRequiresPybindLTOworkaroundGCC>,$<compilerRequiresPybindLTOworkaroundClang>>)

    if(compilerRequiresPybindLTOworkaround)
        message(NOTICE "Disabling Pybind11 LTO for ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMILER_VERSION}.")
    endif()

    # https://pybind11.readthedocs.io/en/stable/compiling.html#pybind11-add-module
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ${compilerRequiresLTOworkaround})

    # apt install python3.9 python3.9-dev
    find_package(Python3 COMPONENTS Interpreter Development)

    if(NOT Python3_FOUND)
        message(FATAL_ERROR "Python3 not found.")
    else()
        message(STATUS "Found python: Using ${Python3_EXECUTABLE} (${Python3_STDLIB})")
    endif()

    # BSD-3-Clause
    FetchContent_Declare(
        pybind11
        URL https://github.com/pybind/pybind11/archive/914c06fb252b6cc3727d0eedab6736e88a3fcb01.zip # v2.9.2
    )
    FetchContent_MakeAvailable(pybind11)

    # BSD-3-Clause
    FetchContent_Declare(
        pybind11_abseil
        URL https://github.com/pybind/pybind11_abseil/archive/38111ef06d426f75bb335a3b58aa0342f6ce0ce3.zip
    )
    FetchContent_MakeAvailable(pybind11_abseil)

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
            set(PYTHON_BINDINGS_EXTRA_LINKER_FLAGS "")
        endif()

        # using regular Clang or AppleClang
    endif()
endif()

# # Google Test
if(HAVE_TESTS)
    # See also: https://google.github.io/googletest/quickstart-cmake.html
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip # release-1.11.0
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# Java Bindings
if(HAVE_JAVA_BINDINGS)
    set(JAVA_HOME $ENV{JAVA_HOME})

    if(JAVA_HOME)
        message(STATUS "Using Java home from JAVA_HOME environment variable: ${JAVA_HOME}")
    else()
        set(JAVA_HOME "/usr/lib/jvm/java-11-openjdk-amd64")
        message(STATUS "Using default Java home ${JAVA_HOME}. Otherwise specify the environment variable JAVA_HOME.")
    endif()

    set(CMAKE_JAVA_COMPILE_FLAGS -source 1.8 -target 1.8)

    find_package(JNI)

    if(NOT JNI_FOUND)
        message(FATAL_ERROR "No Java found, either disable `HAVE_JAVA_BINDINGS` or set `JAVA_HOME` environment variable!")
    endif()

    find_package(Java 1.8 REQUIRED)

    if(NOT JAVA_FOUND)
        message(FATAL_ERROR "No Java found!")
    endif()

    message(STATUS "Java ${Java_VERSION} found: ${JNI_INCLUDE_DIRS} ${JNI_LIBRARIES}")
endif()

if(HAVE_TESTS)
    enable_testing()
    include(GoogleTest)
endif()

# ############################
# ## GEDS Version
find_package(Git)

if(DEFINED ENV{GEDS_VERSION})
    set(GEDS_VERSION_ $ENV{GEDS_VERSION})
elseif(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE GEDS_VERSION_
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else()
    set(GEDS_VERSION_ "Unknown")
endif()

# Strip `v` from build.
STRING(REGEX REPLACE "^v" "" GEDS_VERSION ${GEDS_VERSION_})

message(STATUS "GEDS: Version ${GEDS_VERSION}")

# ###########
add_subdirectory(src)

add_custom_target(build-all
    DEPENDS
    benchmark_io
    shuffle_read
    shuffle_serve
    serve_pubsub
    read_wo_pubsub
    read_with_pubsub
    geds
    geds_cmd
    metadataserver
    test_metadataserver
    pygeds

    geds_java
    geds_jar
)
