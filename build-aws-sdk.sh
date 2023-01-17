#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

ROOT="$(pwd)"
source "${ROOT}/DEPENDENCIES"

BUILD_TYPE=${BUILD_TYPE:-Release}

CPP_VERSION=${CPP_VERSION:-20}

if [ "${BUILD_TYPE}" != "Release" ]; then
    INSTALL_PREFIX=${INSTALL_PREFIX:-"/usr/local/opt/aws-sdk-cpp${CPP_VERSION}_${BUILD_TYPE}"}
else
    INSTALL_PREFIX=${INSTALL_PREFIX:-"/usr/local/opt/aws-sdk-cpp${CPP_VERSION}"}
fi

SRC_DIR=${SRC_DIR:-"${SCRIPT_DIR}/../aws-sdk-cpp-${AWS_SDK_VERSION}"}

BUILD_DIR=$(mktemp -d /tmp/aws-sdk-cpp-build-XXX)

CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD:-20}

cd $BUILD_DIR
git clone -b ${AWS_SDK_VERSION} --recurse-submodules https://github.com/aws/aws-sdk-cpp ${SRC_DIR} || true

cmake $SRC_DIR \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_ONLY=s3 \
    -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
    -DCPP_STANDARD=${CPP_VERSION} \
    -DENABLE_TESTING=OFF -DAUTORUN_UNIT_TESTS=OFF
cmake --build ${BUILD_DIR}
sudo rm -rf ${INSTALL_PREFIX} || true
sudo cmake --install ${BUILD_DIR}

rm -rf "${BUILD_DIR}"
