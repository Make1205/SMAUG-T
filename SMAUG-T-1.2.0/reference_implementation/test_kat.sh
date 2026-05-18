#!/bin/bash

set -e

BIN_DIR=./build/release/bin
REF_DIR=../kat

declare -a MODES=("mode1" "mode3" "mode5" "modet")

echo "==== KAT TEST START ===="

for mode in "${MODES[@]}"; do
    echo "[*] Running smaugt-${mode}-kat..."
    ${BIN_DIR}/smaugt-${mode}-kat

    GEN_FILE=PQCkemKAT_smaugt_${mode}.rsp
    REF_FILE=${REF_DIR}/PQCkemKAT_smaugt_${mode}.rsp

    echo "[*] Comparing ${mode}..."

    if diff -ru "$GEN_FILE" "$REF_FILE" > /dev/null; then
        echo "[PASS] ${mode}"
    else
        echo "[FAIL] ${mode}"
        diff -ru "$GEN_FILE" "$REF_FILE"
    fi
    
    rm "$GEN_FILE" PQCkemKAT_smaugt_${mode}.req

    echo ""
done

echo "==== KAT TEST END ===="
