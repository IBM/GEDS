#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

# See here https://cmake.org/cmake/help/latest/module/UseJava.html for building Java with CMake.
include(UseJava)

SET(JAVA_SOURCES
        com/ibm/geds/GEDS.java
        com/ibm/geds/GEDSConfig.java
        com/ibm/geds/GEDSFile.java
        com/ibm/geds/GEDSFileStatus.java
)
SET(SOURCES
        com_ibm_geds_GEDS.cpp
        com_ibm_geds_GEDSConfig.cpp
        com_ibm_geds_GEDSFile.cpp

        JavaError.cpp
        JavaError.h
        Platform.cpp
)

set(GEDS_JAVA_VERSION 1.1)

set(GEDS_JAR_GROUPID "com.ibm.geds")
set(CMAKE_JAR_ARTIFACT_NAME "geds")
add_jar(geds_jar
        SOURCES ${JAVA_SOURCES}
        GENERATE_NATIVE_HEADERS geds_jar-native
        OUTPUT_NAME ${CMAKE_JAR_ARTIFACT_NAME}
        VERSION ${GEDS_JAVA_VERSION}
        COMPONENT geds
        DESTINATION INSTALL include)
add_library(geds_java SHARED
        ${SOURCES}
)
target_link_libraries(
        geds_java
        PRIVATE
        geds_jar-native
        libgeds)
target_include_directories(geds_java
        PRIVATE
        "${JAVA_INCLUDE_PATH2}"
)
target_compile_options(geds_java PUBLIC ${GEDS_EXTRA_COMPILER_FLAGS})

install(TARGETS geds_java DESTINATION java COMPONENT geds)
install_jar(geds_jar DESTINATION java COMPONENT geds)

# Create a fake Maven installation path to allow local installations.
# Example config:
# <repositories>
# <repository>
# <id>geds</id>
# <name>GEDS</name>
# <url>file:///Users/psp/geds-install/java</url>
# </repository>
# </repositories>
set(GEDS_JAVA_POM ${CMAKE_JAR_ARTIFACT_NAME}-${GEDS_JAVA_VERSION}.pom)
configure_file(pom.xml.template ${GEDS_JAVA_POM})

SET(GEDS_JAVA_INSTALL_PATH
        java/com/ibm/geds/${CMAKE_JAR_ARTIFACT_NAME}/${GEDS_JAVA_VERSION}/)
install(TARGETS geds_java DESTINATION ${GEDS_JAVA_INSTALL_PATH} COMPONENT geds)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${GEDS_JAVA_POM} DESTINATION ${GEDS_JAVA_INSTALL_PATH} COMPONENT geds)
install_jar(geds_jar DESTINATION ${GEDS_JAVA_INSTALL_PATH} COMPONENT geds)
