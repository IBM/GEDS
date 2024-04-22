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

GEDS_VERSION=${GITHUB_TAG:-$(git describe --tags --match "v*" --dirty)}
[[ "$GEDS_VERSION" =~ ^v ]] && GEDS_VERSION=$(echo $GEDS_VERSION | cut -c 2-)

DOCKER=${DOCKER:-"docker"}
DOCKER_IMAGE_PREFIX=${DOCKER_IMAGE_PREFIX:-"zac32.zurich.ibm.com/zrlio/"}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
IMAGE_PREFIX=geds-dev_${DOCKER_BUILD_TARGET}
if [[ "${CMAKE_BUILD_TYPE}" != "Release" ]]; then
    IMAGE_PREFIX=${IMAGE_PREFIX}-${CMAKE_BUILD_TYPE,,}
fi
IMAGE_NAME=${IMAGE_PREFIX}:${GEDS_DOCKER_VERSION}
echo $IMAGE_NAME
IMAGE=${DOCKER_IMAGE_PREFIX}${IMAGE_PREFIX}

BASE_BUILD_IMAGE="geds_build-${DOCKER_BUILD_TARGET}:${GEDS_DOCKER_VERSION}"
$DOCKER buildx build -t ${BASE_BUILD_IMAGE} \
    -f docker/Dockerfile-base_${DOCKER_BUILD_TARGET} .

GRPC_DOCKER_IMAGE="geds_dependencies-${DOCKER_BUILD_TARGET}:${GEDS_DOCKER_VERSION}"
$DOCKER buildx build -t ${GRPC_DOCKER_IMAGE} \
    --build-arg GEDS_DOCKER_VERSION=${GEDS_DOCKER_VERSION} \
    --build-arg DOCKER_BUILD_TARGET=${DOCKER_BUILD_TARGET} \
    --build-arg GRPC_VERSION=${GRPC_VERSION} \
    --build-arg AWS_SDK_VERSION=${AWS_SDK_VERSION} \
    --build-arg BOOST_VERSION=${BOOST_VERSION} \
    --build-arg BOOST_VERSION_=${BOOST_VERSION//./_} \
    --build-arg CMAKE_BUILD_PARALLEL_LEVEL=$(( $(nproc) + 1)) \
    -f docker/Dockerfile-dependencies .

$DOCKER buildx build -t geds-build/${DOCKER_BUILD_TARGET}:${GIT_REVISION} \
    --build-arg GEDS_DOCKER_VERSION=${GEDS_DOCKER_VERSION} \
    --build-arg DOCKER_BUILD_TARGET=${DOCKER_BUILD_TARGET} \
    --build-arg GEDS_VERSION=${GEDS_VERSION} \
    --build-arg DOCKER_BUILD_TARGET=${DOCKER_BUILD_TARGET} \
    --build-arg CMAKE_BUILD_PARALLEL_LEVEL=$(( $(nproc) + 1)) \
    --build-arg CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    -f docker/Dockerfile-build .

$DOCKER buildx build -t ${IMAGE_NAME} \
    --build-arg GIT_REVISION=${GIT_REVISION} \
    -f docker/Dockerfile_${DOCKER_BUILD_TARGET} .

echo "Built ${IMAGE_NAME}"

$DOCKER tag ${IMAGE_NAME} $IMAGE:latest
$DOCKER tag ${IMAGE_NAME} $IMAGE:${GEDS_VERSION}

echo "Built ${IMAGE}:latest"
echo "Built ${IMAGE}:${GEDS_VERSION}"

$DOCKER push $IMAGE:latest
$DOCKER push $IMAGE:${GEDS_VERSION}

echo "Pushed ${IMAGE}:latest"
echo "Pushed ${IMAGE}:${GEDS_VERSION}"
