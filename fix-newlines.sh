#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

BUILD_TYPE=${1:-"Debug"}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "${SCRIPT_DIR}"

FOLDERS=(
    doc
    src
)
for folder in "${FOLDERS[@]}"
do
    find "${folder}" -type f -exec sed -i -e '$a\' {} \;
done

find . -maxdepth 1 -type f -exec sed -i -e '$a\' {} \;
