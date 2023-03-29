#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

if(NOT HAVE_PYTHON_BINDINGS)
    message(error "This module requires the python bindings flag.")
else()
    message(STATUS "Building python library.")
endif()

set(SOURCES
    wrapper.cpp
)

set(PYBIND11_ABSL_SOURCES
    ${pybind11_abseil_SOURCE_DIR}/pybind11_abseil/import_status_module.cc
    ${pybind11_abseil_SOURCE_DIR}/pybind11_abseil/register_status_bindings.cc
)

set(PYTHON_SOURCES
    create.py
    read.py
    folder_listing.py
    metadata.py
    metadata_read.py
    s3.py
    s3_compare_geds.py
    s3_download.py
    serve_benchmark.py
)

# https://pybind11.readthedocs.io/en/stable/cmake/index.html
Python3_add_library(pygeds ${SOURCES} ${PYBIND11_ABSL_SOURCES})

target_link_libraries(pygeds
    PRIVATE
    libgeds
    pybind11::headers)
target_compile_definitions(pygeds
    PUBLIC
    PYBIND11_ABSEIL_STATUS_MODULE_PATH=pygeds.status
)
target_compile_options(pygeds PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS} -Wno-unused-parameter)
target_include_directories(pygeds
    PRIVATE
    ${pybind11_abseil_SOURCE_DIR})
set_target_properties(pygeds
    PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION $<CMAKE_INTERPROCEDURAL_OPTIMIZATION>
    CXX_VISIBILITY_PRESET hidden # https://pybind11.readthedocs.io/en/stable/faq.html#someclass-declared-with-greater-visibility-than-the-type-of-its-field-someclass-member-wattributes
    VISIBLITY_INLINES_HIDDEN ON
)

file(COPY ${PYTHON_SOURCES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

install(FILES ${PYTHON_SOURCES}
    COMPONENT geds
    DESTINATION python/)
install(TARGETS pygeds
    COMPONENT geds
    DESTINATION python/)
