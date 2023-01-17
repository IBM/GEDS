#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache-2.0
#

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

cd "${SCRIPT_DIR}"
find src -name "*.cpp" -o -name "*.h" -exec echo {} \; \
     -exec clang-tidy "-checks=cppcoreguidelines-*,clang-analyzer-*,modernize-*,-modernize-use-trailing-return-type,-cppcoreguidelines-non-private-member-variables-in-classes" {} \;
