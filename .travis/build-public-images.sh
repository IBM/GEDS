#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#
set -euo pipefail

if [[ "${CMAKE_BUILD_TYPE}" != "Release" ]]; then
    exit
fi


SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd "${SCRIPT_DIR}/../"
ROOT="$(pwd)"

curl -sL https://ibm.biz/idt-installer | bash
# ibmcloud login -r 'eu-de' --apikey "${IBMCLOUD_API_KEY}"
# ibmcloud cr login

GEDS_VERSION=${TRAVIS_TAG:-$(git describe --tags --match "v*" --dirty | cut -c 2-)}
GIT_REVISION=$(git rev-parse --short HEAD)
GRPC_DOCKER_IMAGE=${GRPC_DOCKER_IMAGE:-"de.icr.io/geds/${DOCKER_BUILD_TARGET}:${GIT_REVISION}"}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-"Release"}

INSTALL_DIR="${INSTALL_DIR:-"${ROOT}/travis_install"}"

echo "Building ${GRPC_DOCKER_IMAGE}"
docker build -t ${GRPC_DOCKER_IMAGE} -f docker/Dockerfile_${DOCKER_BUILD_TARGET} .
