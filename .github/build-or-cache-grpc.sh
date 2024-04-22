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
DOCKER_CACHE_DIR="${DOCKER_CACHE_DIR:-"${ROOT}/github_actions_cache"}"

GIT_REVISION=$(git rev-parse --short HEAD)
GRPC_DOCKER_IMAGE="geds_dependencies-${DOCKER_BUILD_TARGET}:${GEDS_DOCKER_VERSION}"
GRPC_CACHED="${DOCKER_CACHE_DIR}/docker_geds_dependencies-${DOCKER_BUILD_TARGET}-${GEDS_DOCKER_VERSION}.tgz"

if [ -f "${GRPC_CACHED}" ]; then
    echo "Loading cached docker image from ${GRPC_CACHED}"
    (gzip -dc "${GRPC_CACHED}" | $DOCKER load) || true
else
    echo "Cached docker image does not exist in ${GRPC_CACHED}."
    CMAKE_BUILD_PARALLEL_LEVEL=$(($(nproc) + 1))
    echo "Using ${CMAKE_BUILD_PARALLEL_LEVEL} threads"

    echo "Building base image"
    BASE_BUILD_IMAGE="geds_build-${DOCKER_BUILD_TARGET}:${GEDS_DOCKER_VERSION}"
    $DOCKER buildx build -t ${BASE_BUILD_IMAGE} \
        -f docker/Dockerfile-base_${DOCKER_BUILD_TARGET} .

    echo "Building dependencies"
    $DOCKER buildx build -t ${GRPC_DOCKER_IMAGE} \
         --build-arg GEDS_DOCKER_VERSION=${GEDS_DOCKER_VERSION} \
         --build-arg DOCKER_BUILD_TARGET=${DOCKER_BUILD_TARGET} \
         --build-arg CMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL} \
         --build-arg GRPC_VERSION=${GRPC_VERSION} \
         --build-arg AWS_SDK_VERSION=${AWS_SDK_VERSION} \
         --build-arg BOOST_VERSION=${BOOST_VERSION} \
         --build-arg BOOST_VERSION_=${BOOST_VERSION//./_} \
         -f docker/Dockerfile-dependencies . &> grpc.log || cat grpc.log

    mkdir -p "${DOCKER_CACHE_DIR}"
    $DOCKER save "${GRPC_DOCKER_IMAGE}" | gzip > "${GRPC_CACHED}"
fi
