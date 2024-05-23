#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd $SCRIPT_DIR

export GEDS_METADATASERVER=flex12:4381
export GEDS_TMP=/mnt/psp/GEDS_XXXXXX
export GEDS_AVAILABLE_STORAGE=$(( 600 * 1024 * 1024 * 1024 ))
python $@

