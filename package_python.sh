#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd "${SCRIPT_DIR}"
ROOT="$(pwd)"

GEDS_VERSION=${GEDS_VERSION:-$(git describe --tags --match "v*" --dirty)}
[[ "$GEDS_VERSION" =~ ^v ]] && GEDS_VERSION=$(echo $GEDS_VERSION | cut -c 3-)
GEDS_VERSION=${GEDS_VERSION/-/+} # Workaround for pep440
echo "Building python package with ${GEDS_VERSION}"


BUILD_TARGET=${BUILD_TARGET:-$(lsb_release -d | awk '{print $2 "-"  $3}')}
BUILD_TYPE=${BUILD_TYPE:-"unknown"}

BUILD_TARGET=$(echo ${BUILD_TARGET} | awk '{print tolower($0)}')
BUILD_TYPE=$(echo ${BUILD_TYPE} | awk '{print tolower($0)}')

ARTIFACTS_DIR="${ARTIFACTS_DIR:-"${ROOT}/travis_artifacts"}"
GEDS_INSTALL_PREFIX=${GEDS_INSTALL_PREFIX:-"${ROOT}/travis_install"}
BUILD_LOC=$(mktemp -d /tmp/gedspy_XXX)

cp -a src/python/geds_smart_open/ "${BUILD_LOC}/"
cd "${BUILD_LOC}/geds_smart_open"
cp "${GEDS_INSTALL_PREFIX}/geds/python/pygeds.so" src/geds_smart_open/
sed -i "s/SNAPSHOT/${GEDS_VERSION}/g" "pyproject.toml"

pip install 'build[virtualenv]'
python3 -m build --outdir "${ARTIFACTS_DIR}"
for i in $(ls "${ARTIFACTS_DIR}"/geds_smart_*.tar.gz) ; do
    echo "Renaming ${i} to ${i/.tar.gz/-${BUILD_TARGET}-${BUILD_TYPE}.tar.gz}"
    mv "${i}" "${i/.tar.gz/-${BUILD_TARGET}-${BUILD_TYPE}.tar.gz}"
done
for i in $(ls "${ARTIFACTS_DIR}"/geds_smart_*.whl) ; do
    echo "Renaming ${i} to ${i/.whl/.${BUILD_TARGET}-${BUILD_TYPE}}"
    mv "${i}" "${i/.whl/.whl.${BUILD_TARGET}-${BUILD_TYPE}}"
done
cd "${ROOT}"
rm -rf "${BUILD_LOC}"
