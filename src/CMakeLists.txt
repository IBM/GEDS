#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

# Dependencies for GEDS
add_subdirectory(common)
add_subdirectory(utility)
add_subdirectory(grpchelloworld)
add_subdirectory(protos)
add_subdirectory(statistics)
add_subdirectory(s3)

# Main geds library
add_subdirectory(libgeds)

# Binaries
add_subdirectory(benchmark)
add_subdirectory(geds)
add_subdirectory(geds_cmd)
add_subdirectory(metadataservice)
add_subdirectory(scripts)

if(HAVE_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

if(HAVE_JAVA_BINDINGS)
    add_subdirectory(java)
endif()
