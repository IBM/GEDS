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

GEDS_VERSION=${TRAVIS_TAG:-$(git describe --tags --match "v*" --dirty)}
[[ "$GEDS_VERSION" =~ ^v ]] && GEDS_VERSION=$(echo $GEDS_VERSION | cut -c 3-)

GIT_REVISION=$(git rev-parse --short HEAD)
GRPC_DOCKER_IMAGE=${GRPC_DOCKER_IMAGE:-"geds_dependencies-${DOCKER_BUILD_TARGET}:${GIT_REVISION}"}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-"Release"}

INSTALL_DIR="${INSTALL_DIR:-"${ROOT}/travis_install"}"

mkdir -p install
mkdir -p artifacts
docker run \
    -v "${ROOT}":"/src/geds" \
    -v "${INSTALL_DIR}/geds":"/install" \
    -e GEDS_VERSION=${GEDS_VERSION} \
    -e BUILD_FOLDER="/build/geds" \
    -e INSTALL_PREFIX="/install/" \
    -e CMAKE_BUILD_PARALLEL_LEVEL=8 \
    -e CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -e RUN_TESTS=1 \
    -w "/build/geds" \
    -t $GRPC_DOCKER_IMAGE \
    /src/geds/docker/build.sh

if [ "${TRAVIS:-""}" == "true" ]; then
    echo "Running as travis."
    cd "${INSTALL_DIR}"
    mkdir -p "${ROOT}/travis_artifacts"
    tar cf "${ROOT}/travis_artifacts/geds-x86_64-${DOCKER_BUILD_TARGET}-${GEDS_VERSION}-${CMAKE_BUILD_TYPE}.tar.gz" geds

    docker run \
        -v "${ROOT}":"/src/geds" \
        -v "${INSTALL_DIR}":"/install" \
        -e BUILD_TARGET=${DOCKER_BUILD_TARGET} \
        -e BUILD_TYPE=${CMAKE_BUILD_TYPE} \
        -e GEDS_VERSION=${GEDS_VERSION} \
        -e GEDS_INSTALL_PREFIX="/install/" \
        -e ARTIFACTS_PREFIX="/src/geds/travis_artifacts/" \
        -w "/build/geds" \
        -t docker.io/python:3.10-buster \
        /src/geds/package_python.sh
fi
