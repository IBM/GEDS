#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "${SCRIPT_DIR}"
ROOT="$(pwd)"

source "${ROOT}/DEPENDENCIES"

GIT_REVISION=$(git rev-parse --short HEAD)
git diff-files --quiet || GIT_REVISION="${GIT_REVISION}-dirty"
GEDS_VERSION=${TRAVIS_TAG:-$(git describe --tags --match "v*" --dirty)}
[[ "$GEDS_VERSION" =~ ^v ]] && GEDS_VERSION=$(echo $GEDS_VERSION | cut -c 2-)

DOCKER_BUILD_TARGET="debian11.5"
REGISTRY="${REGISTRY:-zac32.zurich.ibm.com}"
PREFIX="${PREFIX:-${USER}}"
DOCKER_VERSION=latest
IMAGE_NAME="${REGISTRY}/${PREFIX}/geds-dev_${DOCKER_BUILD_TARGET}"

BASE_BUILD_IMAGE="geds_build:${DOCKER_BUILD_TARGET}"
docker build -t ${BASE_BUILD_IMAGE} \
    -f docker/Dockerfile-base_${DOCKER_BUILD_TARGET} .

GRPC_DOCKER_IMAGE="geds_dependencies-${DOCKER_BUILD_TARGET}:${DOCKER_VERSION}"
# comment which lines : all
#docker build -t ${GRPC_DOCKER_IMAGE} \
#    --build-arg CMAKE_BUILD_PARALLEL_LEVEL=$(($(nproc) + 1)) \
#    --build-arg DOCKER_BUILD_TARGET=${DOCKER_BUILD_TARGET} \
#    --build-arg GRPC_VERSION=${GRPC_VERSION} \
#    --build-arg AWS_SDK_VERSION=${AWS_SDK_VERSION} \
#    --build-arg BOOST_VERSION=${BOOST_VERSION} \
#    --build-arg BOOST_VERSION_=${BOOST_VERSION//./_} \
#    -f docker/Dockerfile-dependencies .

docker build -t geds-build_${DOCKER_BUILD_TARGET}:${GIT_REVISION} \
    --build-arg CMAKE_BUILD_PARALLEL_LEVEL=$(($(nproc) + 1)) \
    --build-arg GIT_REVISION=${GIT_REVISION} \
    --build-arg GEDS_VERSION=${GEDS_VERSION} \
    --build-arg DOCKER_BUILD_TARGET=${DOCKER_BUILD_TARGET} \
    -f docker/Dockerfile-build .

REGISTRY="${REGISTRY:-zac32.zurich.ibm.com}"
PREFIX="${PREFIX:-${USER}}"

IMAGE_BUILD="geds-build_${DOCKER_BUILD_TARGET}:${GIT_REVISION}"

docker tag $IMAGE_BUILD ${REGISTRY}/${PREFIX}/geds-build_${DOCKER_BUILD_TARGET}:latest

#docker build -t ${IMAGE_NAME}:${GIT_REVISION} \
#    --build-arg GIT_REVISION=${GIT_REVISION} \
#    -f docker/Dockerfile_${DOCKER_BUILD_TARGET} .

#docker tag ${IMAGE_NAME}:${GIT_REVISION} $IMAGE_NAME:latest
#docker tag ${IMAGE_NAME}:${GIT_REVISION} $IMAGE_NAME:${GEDS_VERSION}
#docker push ${IMAGE_NAME}:latest
#docker push ${IMAGE_NAME}:${GEDS_VERSION}

docker push ${REGISTRY}/${PREFIX}/geds-build_${DOCKER_BUILD_TARGET}:latest








