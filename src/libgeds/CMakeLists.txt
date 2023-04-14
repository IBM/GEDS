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
        GEDSConfig.h
        GEDSConfig.cpp
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

# Create an object lib to build both a dynamic and a static library.
add_library(geds_objlib OBJECT ${SOURCES})
set_target_properties(geds_objlib
        PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(geds_objlib
        PUBLIC
        geds_s3
        ${AWSSDK_LINK_LIBRARIES}
)
target_compile_options(geds_objlib PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})
target_compile_definitions(geds_objlib
        PUBLIC
        "HAVE_RDMA=$<BOOL:${HAVE_RDMA}>"
        _POSIX_C_SOURCE=200809L
)
target_include_directories(geds_objlib
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)
target_link_libraries(geds_objlib
        PUBLIC
        magic_enum::magic_enum
        geds_common
        geds_proto
        geds_utility
        geds_statistics
        geds_s3
        ${GEDS_EXTRA_LINKER_FLAGS}
)
 
# Static library
add_library(libgeds STATIC $<TARGET_OBJECTS:geds_objlib>)
set_target_properties(libgeds
        PROPERTIES
        OUTPUT_NAME "geds"
)
target_link_libraries(libgeds PUBLIC geds_objlib)

# Dynamic library
add_library(libgeds_dynamic SHARED $<TARGET_OBJECTS:geds_objlib>)
set_target_properties(libgeds_dynamic
        PROPERTIES
        OUTPUT_NAME "geds"
)
target_link_libraries(libgeds_dynamic PUBLIC geds_objlib)

install(TARGETS libgeds libgeds_dynamic
        COMPONENT geds)

if(HAVE_TESTS)
        add_subdirectory(tests)
endif()
