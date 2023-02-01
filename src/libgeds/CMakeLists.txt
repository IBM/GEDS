#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

SET(SOURCES
        Filesystem.cpp
        Filesystem.h
        FileTransferProtocol.cpp
        FileTransferProtocol.h
        FileTransferService.cpp
        FileTransferService.h

        TcpTransport.h
        TcpTransport.cpp

        GEDS.cpp
        GEDS.h
        GEDSAbstractFileHandle.cpp
        GEDSAbstractFileHandle.h
        GEDSCachedFileHandle.cpp
        GEDSCachedFileHandle.h
        GEDSFile.h
        GEDSFile.cpp
        GEDSFileHandle.h
        GEDSFileHandle.cpp
        GEDSProtocol.h
        GEDSProtocol.cpp
        GEDSService.cpp
        GEDSService.h
        HttpServer.cpp
        HttpServer.h
        HttpSession.cpp
        HttpSession.h
        MetadataService.cpp
        MetadataService.h
        GEDSInternal.cpp
        GEDSInternal.h
        GEDSLocalFileHandle.h
        GEDSMMapFileHandle.h
        GEDSRemoteFileHandle.cpp
        GEDSRemoteFileHandle.h
        GEDSS3FileHandle.cpp
        GEDSS3FileHandle.h

        LocalFile.cpp
        LocalFile.h
        MMAPFile.cpp
        MMAPFile.h
        Server.cpp
        Server.h
)

add_library(libgeds STATIC ${SOURCES})
target_link_libraries(libgeds
        PUBLIC
        magic_enum::magic_enum
        geds_common
        geds_proto
        geds_utility
        geds_statistics
        geds_s3
)

target_link_libraries(libgeds
        PUBLIC
        geds_s3
        ${AWSSDK_LINK_LIBRARIES}
)

target_compile_options(libgeds PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
target_compile_definitions(libgeds
        PUBLIC
        "HAVE_RDMA=$<BOOL:${HAVE_RDMA}>"
        _POSIX_C_SOURCE=200809L
)
target_include_directories(libgeds
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(libgeds
        PROPERTIES
        OUTPUT_NAME "geds"
        POSITION_INDEPENDENT_CODE ON
)

install(TARGETS libgeds
        COMPONENT geds)

if(HAVE_TESTS)
        add_subdirectory(tests)
endif()
