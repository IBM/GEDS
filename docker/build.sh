#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd "${SCRIPT_DIR}/../"
ROOT="$(pwd)"

echo "Building ${GEDS_VERSION}"

SOURCE_FOLDER="${ROOT}"
BUILD_FOLDER="${BUILD_FOLDER:-"/tmp/build"}"
INSTALL_PREFIX="${INSTALL_PREFIX:-"/tmp/install"}"
RUN_TESTS=${RUN_TESTS:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-"Release"}

mkdir -p "${BUILD_FOLDER}"
cd "${BUILD_FOLDER}"

cmake -S "${SOURCE_FOLDER}" -B "${BUILD_FOLDER}" -G Ninja \
         -DUSE_EXTERNAL_GRPC=ON \
         -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
         -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}"

cmake --build . --target all

if (( $RUN_TESTS )); then
    ctest || (sleep 5 && ctest --repeat until-pass:2 --rerun-failed)
fi

echo "Installing into ${INSTALL_PREFIX}"
cmake --install . --prefix="${INSTALL_PREFIX}"
