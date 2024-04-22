#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd "${SCRIPT_DIR}/../"
ROOT="$(pwd)"

source "${ROOT}/DEPENDENCIES"

DOCKER=${DOCKER:-"docker"}
GEDS_VERSION=${GITHUB_TAG:-$(git describe --tags --match "v*" --dirty)}
[[ "$GEDS_VERSION" =~ ^v ]] && GEDS_VERSION=$(echo $GEDS_VERSION | cut -c 2-)

GIT_REVISION=$(git rev-parse --short HEAD)
GRPC_DOCKER_IMAGE=${GRPC_DOCKER_IMAGE:-"geds_dependencies-${DOCKER_BUILD_TARGET}:${GEDS_DOCKER_VERSION}"}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-"Release"}

INSTALL_DIR="${INSTALL_DIR:-"${ROOT}/github_install_${DOCKER_BUILD_TARGET}"}"

mkdir -p "${INSTALL_DIR}/geds"

$DOCKER run \
    -v "${ROOT}":"/src/geds" \
    -v "${INSTALL_DIR}/geds":"/install" \
    -e GEDS_VERSION=${GEDS_VERSION} \
    -e BUILD_FOLDER="/build/geds" \
    -e INSTALL_PREFIX="/install/" \
    -e CMAKE_BUILD_PARALLEL_LEVEL=$(($(nproc) + 1)) \
    -e CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -e RUN_TESTS=1 \
    -w "/build/geds" \
    -t $GRPC_DOCKER_IMAGE \
    /src/geds/docker/build.sh

if [ "${GITHUB_ACTIONS:-""}" == "true" ]; then
    echo "Running as Github CI."
    cd "${INSTALL_DIR}"
    mkdir -p "${ROOT}/github_artifacts"
    tar cf "${ROOT}/github_artifacts/geds-x86_64-${DOCKER_BUILD_TARGET}-${GEDS_VERSION}-${CMAKE_BUILD_TYPE}.tar.gz" geds

    $DOCKER run \
        -v "${ROOT}":"/src/geds" \
        -v "${INSTALL_DIR}":"/install" \
        -e BUILD_TARGET=${DOCKER_BUILD_TARGET} \
        -e BUILD_TYPE=${CMAKE_BUILD_TYPE} \
        -e GEDS_VERSION=${GEDS_VERSION} \
        -e GEDS_INSTALL_PREFIX="/install/geds" \
        -e ARTIFACTS_PREFIX="/src/geds/github_artifacts/" \
        -w "/build/geds" \
        -t docker.io/python:3.10-buster \
        /src/geds/package_python.sh
fi
