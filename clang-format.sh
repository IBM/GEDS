#!/usr/bin/env bash
#
# Copyright 2022- IBM Inc. All rights reserved
# SPDX-License-Identifier: Apache2.0
#

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

case "$(uname -s)" in
  Darwin*)
    export PATH="/usr/local/opt/llvm@13/bin/:${PATH}"
    CLANG_FORMAT=$(which clang-format)
    if [ -z "${CLANG_FORMAT}" ]; then
      echo "Clang-format is not installed. 'brew install llvm'. Expecting llvm 13.x."
      exit 1
    fi
    ;;
  Linux*)
    CLANG_FORMAT=$(which clang-format-14)
    if [ -z "${CLANG_FORMAT}" ]; then
      CLANG_FORMAT=$(which clang-format-13)
      if [ -z "${CLANG_FORMAT}" ]; then
        echo "Expecting clang-format 13 or 14. Install it with https://apt.llvm.org/ ."
        exit 1
      fi
    fi
    ;;
  *)
    echo "Unsupported operating system $(uname -s)."
    exit 1
    ;;
esac
EXTENSIONS=(
    '*.c'
    '*.cpp'
    '*.h'
    '*.proto'
    )

for EXT in "${EXTENSIONS[@]}"; 
do
    find "${SCRIPT_DIR}/src" -name "${EXT}" -exec "${CLANG_FORMAT}" -i {} \;
done
