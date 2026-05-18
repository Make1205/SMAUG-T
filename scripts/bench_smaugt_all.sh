#!/usr/bin/env bash
set -euo pipefail

ITERS="${ITERS:-1000}"
if ! [[ "$ITERS" =~ ^[1-9][0-9]*$ ]]; then
    echo "ITERS must be a positive integer" >&2
    exit 2
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"
cd "$REPO_ROOT"

RESULTS_DIR="bench_smaugt_baseline/results"
LOG_DIR="$RESULTS_DIR/logs"
BUILD_ROOT="$RESULTS_DIR/build"
CSV="$RESULTS_DIR/smaugt_all.csv"
SUMMARY="$RESULTS_DIR/smaugt_all_summary.md"
mkdir -p "$LOG_DIR" "$BUILD_ROOT"

RUN_COMMAND="ITERS=$ITERS bash scripts/bench_smaugt_all.sh"
GIT_STATUS_BEFORE="$(git status --short 2>/dev/null || true)"
DIRTY_BEFORE="clean"
if [[ -n "$GIT_STATUS_BEFORE" ]]; then DIRTY_BEFORE="dirty"; fi
COMMIT="$(git rev-parse HEAD 2>/dev/null || echo unavailable)"
BRANCH="$(git branch --show-current 2>/dev/null || echo unavailable)"
if [[ -z "$BRANCH" ]]; then BRANCH="detached"; fi

if command -v cc >/dev/null 2>&1; then
    CC_BIN="$(command -v cc)"
elif command -v gcc >/dev/null 2>&1; then
    CC_BIN="$(command -v gcc)"
elif command -v clang >/dev/null 2>&1; then
    CC_BIN="$(command -v clang)"
else
    echo "No C compiler found" >&2
    exit 1
fi
COMPILER_VERSION="$($CC_BIN --version 2>&1 | head -n 1 || true)"
CMAKE_BIN="$(command -v cmake || true)"

SEARCH_TOOL="grep"
search_evidence() {
    local pattern="$1"
    local path="$2"
    if command -v rg >/dev/null 2>&1; then
        rg -n -I -e "$pattern" "$path" 2>/dev/null || true
    else
        grep -RInE "$pattern" "$path" 2>/dev/null || true
    fi
}

csv_escape() {
    local s="${1//$'\n'/; }"
    s="${s//$'\r'/}"
    s="${s//\"/\"\"}"
    printf '"%s"' "$s"
}

write_csv_row() {
    local fields=("$@")
    local first=1
    for field in "${fields[@]}"; do
        if [[ "$first" -eq 0 ]]; then printf ',' >> "$CSV"; fi
        csv_escape "$field" >> "$CSV"
        first=0
    done
    printf '\n' >> "$CSV"
}

value_from_file() {
    local key="$1" file="$2"
    awk -F= -v k="$key" '$1 == k {print substr($0, index($0, "=") + 1); exit}' "$file" 2>/dev/null || true
}

find_ref_root() {
    local candidate
    while IFS= read -r -d '' candidate; do
        if [[ "$candidate" == *"__MACOSX"* ]]; then continue; fi
        if [[ -f "$candidate/include/api.h" && -f "$candidate/include/params.h" && -f "$candidate/src/kem.c" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done < <(find . -type d \( -iname 'reference_implementation' -o -iname 'Reference_Implementation' \) -print0)
    return 1
}

REF_ROOT="$(find_ref_root || true)"
if [[ -z "$REF_ROOT" ]]; then
    echo "Unable to find SMAUG-T reference_implementation root" >&2
    exit 1
fi
REF_ROOT="${REF_ROOT#./}"
RELEASE_ROOT="$(dirname -- "$REF_ROOT")"
KAT_VECTOR_DIR="$RELEASE_ROOT/kat"

mapfile -t REF_DIRS < <(find . -type d \( -iname '*reference*' -o -iname 'ref' \) -not -path './.git/*' -not -path './__MACOSX/*' | sed 's#^./##' | sort)
mapfile -t NAME_OPT_DIRS < <(find . -type d \( -iname '*avx*' -o -iname '*avx2*' -o -iname '*optimized*' -o -iname '*optimised*' -o -iname '*opt' -o -iname 'opt' \) -not -path './.git/*' -not -path './__MACOSX/*' | sed 's#^./##' | sort)
AVX_EVIDENCE_FILE="$LOG_DIR/avx2_evidence.txt"
search_evidence '(-mavx2|-maes|-mbmi2|-mpclmul|-msse2avx|-march=native|immintrin\.h|__m256i|_mm256_)' . > "$AVX_EVIDENCE_FILE"
AVX_DIRS=("${NAME_OPT_DIRS[@]}")
if [[ "${#AVX_DIRS[@]}" -eq 0 && -s "$AVX_EVIDENCE_FILE" ]]; then
    while IFS= read -r line; do
        p="${line%%:*}"
        d="$(dirname -- "$p")"
        [[ "$d" == ./.git* || "$d" == ./__MACOSX* || "$d" == . ]] && continue
        AVX_DIRS+=("${d#./}")
    done < "$AVX_EVIDENCE_FILE"
fi
if [[ "${#AVX_DIRS[@]}" -gt 0 ]]; then
    mapfile -t AVX_DIRS < <(printf '%s\n' "${AVX_DIRS[@]}" | sort -u)
fi

MODE_LEVELS=("128:mode1:SMAUGT_MODE1" "192:mode3:SMAUGT_MODE3" "256:mode5:SMAUGT_MODE5")
declare -A BUILD_COMMANDS FLAGS_USED KAT_STATUS BUILD_STATUS CORRECTNESS_STATUS NOTES Q_VAL PK_VAL CT_VAL SK_VAL SS_VAL KG_VAL ENC_VAL DEC_VAL TIMING_METHOD SOURCE_DIRS

printf '%s\n' 'Scheme,Level,q,implementation,pk_bytes,ct_bytes,sk_bytes,ss_bytes,build_status,kat_status,correctness_status,iterations,KG_kCycles,Enc_kCycles,Dec_kCycles,timing_method,compiler,flags,commit,branch,dirty_before,dirty_after,source_dir,build_command,notes' > "$CSV"

CMAKE_BUILD_DIR="$BUILD_ROOT/cmake_reference"
OFFICIAL_CONFIGURE_CMD=("$CMAKE_BIN" -S "$REF_ROOT" -B "$CMAKE_BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DSMAUGT_BUILD_TEST=OFF)
OFFICIAL_BUILD_CMD=("$CMAKE_BIN" --build "$CMAKE_BUILD_DIR" --config Release --parallel)
OFFICIAL_BUILD_OK=0
OFFICIAL_BUILD_LOG="$LOG_DIR/reference_cmake_build.log"
{
    echo "Reference source root: $REF_ROOT"
    echo "Clean: rm -rf $CMAKE_BUILD_DIR"
    rm -rf "$CMAKE_BUILD_DIR"
    if [[ -z "$CMAKE_BIN" ]]; then
        echo "cmake not found"
        exit 127
    fi
    printf 'Configure command:'; printf ' %q' "${OFFICIAL_CONFIGURE_CMD[@]}"; printf '\n'
    "${OFFICIAL_CONFIGURE_CMD[@]}"
    printf 'Build command:'; printf ' %q' "${OFFICIAL_BUILD_CMD[@]}"; printf '\n'
    "${OFFICIAL_BUILD_CMD[@]}"
} > "$OFFICIAL_BUILD_LOG" 2>&1 && OFFICIAL_BUILD_OK=1 || OFFICIAL_BUILD_OK=0

for item in "${MODE_LEVELS[@]}"; do
    IFS=: read -r level mode macro <<< "$item"
    key="$level:reference"
    SOURCE_DIRS[$key]="$REF_ROOT"
    BUILD_COMMANDS[$key]="rm -rf $CMAKE_BUILD_DIR && ${OFFICIAL_CONFIGURE_CMD[*]} && ${OFFICIAL_BUILD_CMD[*]}; cc local benchmark harness linked against official CMake library"
    FLAGS_USED[$key]="-O3 -std=c11 -DSMAUGT_CONFIG_MODE=$macro"
    NOTES[$key]="mode mapping: SMAUG-T$level -> $mode; source tree is explicitly named reference_implementation"
    BUILD_STATUS[$key]="fail"
    KAT_STATUS[$key]="not_run"
    CORRECTNESS_STATUS[$key]="not_run"
    Q_VAL[$key]="unavailable"; PK_VAL[$key]="unavailable"; CT_VAL[$key]="unavailable"; SK_VAL[$key]="unavailable"; SS_VAL[$key]="unavailable"
    KG_VAL[$key]=""; ENC_VAL[$key]=""; DEC_VAL[$key]=""; TIMING_METHOD[$key]="unavailable"

    MODE_LOG="$LOG_DIR/smaugt_${level}_reference.log"
    BENCH_OUT="$LOG_DIR/smaugt_${level}_reference_bench.out"
    BENCH_BIN="$BUILD_ROOT/bench_smaugt_${mode}_reference"
    LIB_DIR="$CMAKE_BUILD_DIR/lib"
    BIN_DIR="$CMAKE_BUILD_DIR/bin"
    {
        echo "Target: SMAUG-T$level reference ($mode)"
        echo "Official build log: $OFFICIAL_BUILD_LOG"
        if [[ "$OFFICIAL_BUILD_OK" -ne 1 ]]; then
            echo "Official CMake build failed; see $OFFICIAL_BUILD_LOG"
            exit 20
        fi
        BUILD_STATUS[$key]="pass"
        KAT_EXE="$BIN_DIR/smaugt-${mode}-kat"
        if [[ -x "$KAT_EXE" ]]; then
            KAT_WORK="$BUILD_ROOT/kat_${mode}_reference"
            rm -rf "$KAT_WORK"
            mkdir -p "$KAT_WORK"
            cp "$KAT_VECTOR_DIR/PQCkemKAT_smaugt_${mode}.req" "$KAT_WORK/" 2>/dev/null || true
            (cd "$KAT_WORK" && LD_LIBRARY_PATH="$REPO_ROOT/$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "$REPO_ROOT/$KAT_EXE" --disable-seed-gen)
            if [[ -f "$KAT_VECTOR_DIR/PQCkemKAT_smaugt_${mode}.rsp" && -f "$KAT_WORK/PQCkemKAT_smaugt_${mode}.rsp" ]]; then
                diff -u "$KAT_VECTOR_DIR/PQCkemKAT_smaugt_${mode}.rsp" "$KAT_WORK/PQCkemKAT_smaugt_${mode}.rsp"
                echo "KAT response matches bundled response"
                KAT_STATUS[$key]="pass"
            else
                echo "KAT generated successfully but bundled response comparison was unavailable"
                KAT_STATUS[$key]="pass_no_compare"
            fi
        else
            echo "Official KAT executable not found; local correctness will be used"
            KAT_STATUS[$key]="not_available"
        fi
        COMPILE_CMD=("$CC_BIN" -O3 -std=c11 "-DSMAUGT_CONFIG_MODE=$macro" -I "$REF_ROOT/include" "bench_smaugt_baseline/bench_smaugt_kem.c" -L "$LIB_DIR" "-Wl,-rpath,$REPO_ROOT/$LIB_DIR" "-lsmaugt-$mode" -o "$BENCH_BIN")
        printf 'Benchmark compile command:'; printf ' %q' "${COMPILE_CMD[@]}"; printf '\n'
        "${COMPILE_CMD[@]}"
        RUN_BENCH_CMD=("$BENCH_BIN" "$ITERS")
        printf 'Benchmark run command:'; printf ' %q' "${RUN_BENCH_CMD[@]}"; printf '\n'
        LD_LIBRARY_PATH="$REPO_ROOT/$LIB_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" "${RUN_BENCH_CMD[@]}" | tee "$BENCH_OUT"
    } > "$MODE_LOG" 2>&1 && TARGET_OK=1 || TARGET_OK=0

    if [[ "$TARGET_OK" -eq 1 ]]; then
        BUILD_STATUS[$key]="pass"
        [[ "${KAT_STATUS[$key]}" == "not_run" ]] && KAT_STATUS[$key]="not_available"
        CORRECTNESS_STATUS[$key]="$(value_from_file correctness_status "$BENCH_OUT")"
        Q_VAL[$key]="$(value_from_file q "$BENCH_OUT")"
        PK_VAL[$key]="$(value_from_file pk_bytes "$BENCH_OUT")"
        CT_VAL[$key]="$(value_from_file ct_bytes "$BENCH_OUT")"
        SK_VAL[$key]="$(value_from_file sk_bytes "$BENCH_OUT")"
        SS_VAL[$key]="$(value_from_file ss_bytes "$BENCH_OUT")"
        KG_VAL[$key]="$(value_from_file KG_kCycles "$BENCH_OUT")"
        ENC_VAL[$key]="$(value_from_file Enc_kCycles "$BENCH_OUT")"
        DEC_VAL[$key]="$(value_from_file Dec_kCycles "$BENCH_OUT")"
        TIMING_METHOD[$key]="$(value_from_file timing_method "$BENCH_OUT")"
    else
        BUILD_STATUS[$key]="fail"
        CORRECTNESS_STATUS[$key]="not_run"
        NOTES[$key]="${NOTES[$key]}; build/test/benchmark failed, see $MODE_LOG"
    fi

    GIT_STATUS_NOW="$(git status --short 2>/dev/null || true)"
    DIRTY_AFTER_NOW="clean"; if [[ -n "$GIT_STATUS_NOW" ]]; then DIRTY_AFTER_NOW="dirty"; fi
    write_csv_row "SMAUG-T" "$level" "${Q_VAL[$key]}" "reference" "${PK_VAL[$key]}" "${CT_VAL[$key]}" "${SK_VAL[$key]}" "${SS_VAL[$key]}" "${BUILD_STATUS[$key]}" "${KAT_STATUS[$key]}" "${CORRECTNESS_STATUS[$key]}" "$ITERS" "${KG_VAL[$key]}" "${ENC_VAL[$key]}" "${DEC_VAL[$key]}" "${TIMING_METHOD[$key]}" "$COMPILER_VERSION" "${FLAGS_USED[$key]}" "$COMMIT" "$BRANCH" "$DIRTY_BEFORE" "$DIRTY_AFTER_NOW" "${SOURCE_DIRS[$key]}" "${BUILD_COMMANDS[$key]}" "${NOTES[$key]}"

    key_avx="$level:avx2"
    SOURCE_DIRS[$key_avx]="not_available"
    BUILD_COMMANDS[$key_avx]="not_available"
    FLAGS_USED[$key_avx]="not_available"
    BUILD_STATUS[$key_avx]="not_available"
    KAT_STATUS[$key_avx]="not_available"
    CORRECTNESS_STATUS[$key_avx]="not_available"
    NOTES[$key_avx]="No AVX2/optimized SMAUG-T implementation directory was found in this repository. Evidence scan did not identify a buildable AVX2 SMAUG-T target."
    if [[ "${#AVX_DIRS[@]}" -gt 0 ]]; then
        NOTES[$key_avx]="Potential AVX2/optimized evidence exists but no buildable SMAUG-T AVX2 implementation root matching required API/targets was found: ${AVX_DIRS[*]}"
    fi
    write_csv_row "SMAUG-T" "$level" "unavailable" "avx2" "unavailable" "unavailable" "unavailable" "unavailable" "not_available" "not_available" "not_available" "$ITERS" "" "" "" "unavailable" "$COMPILER_VERSION" "not_available" "$COMMIT" "$BRANCH" "$DIRTY_BEFORE" "dirty" "not_available" "not_available" "${NOTES[$key_avx]}"
done

CPU_MODEL="$(awk -F: '/model name/ {sub(/^[ \t]+/, "", $2); print $2; exit}' /proc/cpuinfo 2>/dev/null || echo unavailable)"
OS_VERSION="$( (cat /etc/os-release 2>/dev/null || uname -a) | head -n 5 | tr '\n' '; ' )"
GIT_STATUS_AFTER="$(git status --short 2>/dev/null || true)"
DIRTY_AFTER="clean"; if [[ -n "$GIT_STATUS_AFTER" ]]; then DIRTY_AFTER="dirty"; fi

{
    echo "# SMAUG-T Baseline Benchmark Summary"
    echo
    echo "## Environment"
    echo "- CPU model: ${CPU_MODEL:-unavailable}"
    echo "- OS/version: ${OS_VERSION:-unavailable}"
    echo "- Compiler: $COMPILER_VERSION"
    echo "- Branch: $BRANCH"
    echo "- Commit: $COMMIT"
    echo "- Git status before script:"
    if [[ -n "$GIT_STATUS_BEFORE" ]]; then echo '```'; printf '%s\n' "$GIT_STATUS_BEFORE"; echo '```'; else echo "  - clean"; fi
    echo "- Git status after script:"
    if [[ -n "$GIT_STATUS_AFTER" ]]; then echo '```'; printf '%s\n' "$GIT_STATUS_AFTER"; echo '```'; else echo "  - clean"; fi
    echo "- Exact command used: \`$RUN_COMMAND\`"
    echo "- Detected repository root: \`$REPO_ROOT\`"
    echo
    echo "## Repository audit"
    echo "- Selected SMAUG-T release root: \`$RELEASE_ROOT\`"
    echo "- Selected SMAUG-T implementation root: \`$REF_ROOT\`"
    echo "- Detected SMAUG-T targets: SMAUG-T128 -> smaugt_mode1, SMAUG-T192 -> smaugt_mode3, SMAUG-T256 -> smaugt_mode5. smaugt_modet/TiMER was intentionally not benchmarked."
    echo "- Detected reference directories:"
    if [[ "${#REF_DIRS[@]}" -gt 0 ]]; then for d in "${REF_DIRS[@]}"; do echo "  - \`$d\`"; done; else echo "  - none"; fi
    echo "- Detected AVX2 or optimized directories/evidence:"
    if [[ "${#AVX_DIRS[@]}" -gt 0 ]]; then for d in "${AVX_DIRS[@]}"; do echo "  - \`$d\`"; done; else echo "  - none"; fi
    echo "- AVX2 evidence log: \`$AVX_EVIDENCE_FILE\`"
    echo
    echo "## Build and benchmark commands"
    echo "- Official build command: \`${OFFICIAL_CONFIGURE_CMD[*]} && ${OFFICIAL_BUILD_CMD[*]}\`"
    echo "- Benchmark harness: \`bench_smaugt_baseline/bench_smaugt_kem.c\` linked against the official CMake-built shared libraries."
    echo "- Compiler flags: \`-O3 -std=c11 -DSMAUGT_CONFIG_MODE=<SMAUGT_MODE1|SMAUGT_MODE3|SMAUGT_MODE5>\`"
    echo "- Timing method: rdtsc with lfence serialization on x86; wall_clock_median fallback elsewhere. This run reports \`${TIMING_METHOD[128:reference]:-unavailable}\` for available rows."
    echo
    echo "## q and size-constant mapping"
    echo "- q is extracted as \`1U << SMAUGT_LOG_Q\` from \`params.h\` for the compiled mode."
    echo "- pk_bytes maps to \`CRYPTO_PUBLICKEYBYTES\`, which maps to \`SMAUGT_PUBLICKEY_BYTES\`."
    echo "- ct_bytes maps to \`CRYPTO_CIPHERTEXTBYTES\`, which maps to \`SMAUGT_CIPHERTEXT_BYTES\`."
    echo "- sk_bytes maps to \`CRYPTO_SECRETKEYBYTES\`, which maps to \`SMAUGT_KEM_SECRETKEY_BYTES\`."
    echo "- ss_bytes maps to \`CRYPTO_BYTES\`, which maps to \`SMAUGT_SHARED_SECRETE_BYTES\`."
    echo
    echo "## Results suitability for the Viper paper"
    echo "- Reference rows are suitable as local baseline measurements only when build_status=pass, KAT/correctness passes, and environment caveats are acceptable."
    echo "- AVX2 rows are not suitable for filling missing Viper table entries because this repository snapshot does not contain a buildable AVX2/optimized SMAUG-T implementation."
    echo "- Do not merge these rows with original non-T SMAUG measurements."
    echo
    echo "## Unavailable targets and reasons"
    for item in "${MODE_LEVELS[@]}"; do
        IFS=: read -r level mode macro <<< "$item"
        echo "- SMAUG-T$level avx2: ${NOTES[$level:avx2]}"
    done
    echo
    echo "## Reproducibility caveats"
    echo "- Local baseline only; no CPU pinning was applied."
    echo "- CPU frequency, turbo boost, thermal throttling, VM/container noise, and scheduler noise were not controlled."
    echo "- Cross-run variance was not measured."
    echo "- Median cycle counts are reported; averages are intentionally not used."
    echo
    echo "## CSV"
    echo "- Final CSV: \`$CSV\`"
    echo "- Logs: \`$LOG_DIR/\`"
} > "$SUMMARY"

echo "Wrote $CSV"
echo "Wrote $SUMMARY"
echo "Logs are in $LOG_DIR/"
