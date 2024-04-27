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

if [ "${BUILD_TYPE}" != "Release" ]; then
    INSTALL_PREFIX=${INSTALL_PREFIX:-"/usr/local/opt/boost-${BOOST_VERSION}_${BUILD_TYPE}"}
else
    INSTALL_PREFIX=${INSTALL_PREFIX:-"/usr/local/opt/boost-${BOOST_VERSION}"}
fi

BUILD_DIR=$(mktemp -d /tmp/boost-${BOOST_VERSION}-build-XXX)
mkdir -p $BUILD_DIR
wget -O ${BUILD_DIR}/boost.tar.gz https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz
cd "${BUILD_DIR}"
tar xf boost.tar.gz --strip-components 1
./bootstrap.sh --prefix="${INSTALL_PREFIX}"
sudo ./b2 cxxflags=-fPIC cflags=-fPIC install
