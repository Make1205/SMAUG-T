#!/usr/bin/env bash
set -euo pipefail

SCRIPT="./parse_speed.sh"

MODE_LIST=("mode1" "mode3" "mode5" "modet")

for mode in "${MODE_LIST[@]}"; do
    echo "${mode}"
    echo "$1"
    "$SCRIPT" "$mode"
    echo   # 빈 줄 (가독성)
done
