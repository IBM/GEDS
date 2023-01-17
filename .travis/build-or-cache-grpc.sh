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

DOCKER_CACHE_DIR="${DOCKER_CACHE_DIR:-"${ROOT}/travis_cache"}"

GIT_REVISION=$(git rev-parse --short HEAD)
GRPC_DOCKER_IMAGE="geds_dependencies-${DOCKER_BUILD_TARGET}:grpc${GRPC_VERSION}-aws${AWS_SDK_VERSION}-boost${BOOST_VERSION}"
GRPC_CACHED="${DOCKER_CACHE_DIR}/docker_geds_dependencies-${DOCKER_BUILD_TARGET}-${GRPC_VERSION}-boost${BOOST_VERSION}.tgz"

if [ -f "${GRPC_CACHED}" ]; then
    echo "Loading cached docker image from ${GRPC_CACHED}"
    (gzip -dc "${GRPC_CACHED}" | docker load) || true
    docker tag ${GRPC_DOCKER_IMAGE} geds_dependencies-${DOCKER_BUILD_TARGET}:${GIT_REVISION}
else
    echo "Cached docker image does not exist in ${GRPC_CACHED}."

    BASE_BUILD_IMAGE="geds_build:${DOCKER_BUILD_TARGET}"
    docker build -t ${BASE_BUILD_IMAGE} \
        -f docker/Dockerfile-base_${DOCKER_BUILD_TARGET} .

    docker build -t ${GRPC_DOCKER_IMAGE} \
         --build-arg DOCKER_BUILD_TARGET=${DOCKER_BUILD_TARGET} \
         --build-arg GRPC_VERSION=${GRPC_VERSION} \
         --build-arg AWS_SDK_VERSION=${AWS_SDK_VERSION} \
         --build-arg BOOST_VERSION=${BOOST_VERSION} \
         --build-arg BOOST_VERSION_=${BOOST_VERSION//./_} \
         -f docker/Dockerfile-dependencies .
    docker tag ${GRPC_DOCKER_IMAGE} geds_dependencies-${DOCKER_BUILD_TARGET}:${GIT_REVISION}
    docker tag ${GRPC_DOCKER_IMAGE} geds_dependencies-${DOCKER_BUILD_TARGET}:latest

    mkdir -p "${DOCKER_CACHE_DIR}"
    docker save "${GRPC_DOCKER_IMAGE}" | gzip > "${GRPC_CACHED}"
fi
