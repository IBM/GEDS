#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

if (NOT HAVE_METADATASERVICE_GO)
    message(error "This module requires the metadata service go flag.")
else ()
    message(STATUS "Building metadata service go.")
endif ()


set(GO_SOURCES
        configs/app.env.secret
        )

file(COPY ${GO_SOURCES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
