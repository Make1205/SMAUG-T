# SMAUG-T Baseline Benchmark Summary

## Environment
- CPU model: Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz
- OS/version: PRETTY_NAME="Ubuntu 24.04.4 LTS";NAME="Ubuntu";VERSION_ID="24.04";VERSION="24.04.4 LTS (Noble Numbat)";VERSION_CODENAME=noble;
- Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Branch: work
- Commit: 724faac0a484851588417e1ccfd86885883dc1bb
- Git status before script:
```
?? bench_smaugt_baseline/
?? scripts/
```
- Git status after script:
```
?? bench_smaugt_baseline/
?? scripts/
```
- Exact command used: `ITERS=1000 bash scripts/bench_smaugt_all.sh`
- Detected repository root: `/workspace/SMAUG-T`

## Repository audit
- Selected SMAUG-T release root: `SMAUG-T-1.2.0`
- Selected SMAUG-T implementation root: `SMAUG-T-1.2.0/reference_implementation`
- Detected SMAUG-T targets: SMAUG-T128 -> smaugt_mode1, SMAUG-T192 -> smaugt_mode3, SMAUG-T256 -> smaugt_mode5. smaugt_modet/TiMER was intentionally not benchmarked.
- Detected reference directories:
  - `SMAUG-T-1.2.0/reference_implementation`
- Detected AVX2 or optimized directories/evidence:
  - none
- AVX2 evidence log: `bench_smaugt_baseline/results/logs/avx2_evidence.txt`

## Build and benchmark commands
- Official build command: `/usr/bin/cmake -S SMAUG-T-1.2.0/reference_implementation -B bench_smaugt_baseline/results/build/cmake_reference -DCMAKE_BUILD_TYPE=Release -DSMAUGT_BUILD_TEST=OFF && /usr/bin/cmake --build bench_smaugt_baseline/results/build/cmake_reference --config Release --parallel`
- Benchmark harness: `bench_smaugt_baseline/bench_smaugt_kem.c` linked against the official CMake-built shared libraries.
- Compiler flags: `-O3 -std=c11 -DSMAUGT_CONFIG_MODE=<SMAUGT_MODE1|SMAUGT_MODE3|SMAUGT_MODE5>`
- Timing method: rdtsc with lfence serialization on x86; wall_clock_median fallback elsewhere. This run reports `rdtsc_lfence_median` for available rows.

## q and size-constant mapping
- q is extracted as `1U << SMAUGT_LOG_Q` from `params.h` for the compiled mode.
- pk_bytes maps to `CRYPTO_PUBLICKEYBYTES`, which maps to `SMAUGT_PUBLICKEY_BYTES`.
- ct_bytes maps to `CRYPTO_CIPHERTEXTBYTES`, which maps to `SMAUGT_CIPHERTEXT_BYTES`.
- sk_bytes maps to `CRYPTO_SECRETKEYBYTES`, which maps to `SMAUGT_KEM_SECRETKEY_BYTES`.
- ss_bytes maps to `CRYPTO_BYTES`, which maps to `SMAUGT_SHARED_SECRETE_BYTES`.

## Results suitability for the Viper paper
- Reference rows are suitable as local baseline measurements only when build_status=pass, KAT/correctness passes, and environment caveats are acceptable.
- AVX2 rows are not suitable for filling missing Viper table entries because this repository snapshot does not contain a buildable AVX2/optimized SMAUG-T implementation.
- Do not merge these rows with original non-T SMAUG measurements.

## Unavailable targets and reasons
- SMAUG-T128 avx2: No AVX2/optimized SMAUG-T implementation directory was found in this repository. Evidence scan did not identify a buildable AVX2 SMAUG-T target.
- SMAUG-T192 avx2: No AVX2/optimized SMAUG-T implementation directory was found in this repository. Evidence scan did not identify a buildable AVX2 SMAUG-T target.
- SMAUG-T256 avx2: No AVX2/optimized SMAUG-T implementation directory was found in this repository. Evidence scan did not identify a buildable AVX2 SMAUG-T target.

## Reproducibility caveats
- Local baseline only; no CPU pinning was applied.
- CPU frequency, turbo boost, thermal throttling, VM/container noise, and scheduler noise were not controlled.
- Cross-run variance was not measured.
- Median cycle counts are reported; averages are intentionally not used.

## CSV
- Final CSV: `bench_smaugt_baseline/results/smaugt_all.csv`
- Logs: `bench_smaugt_baseline/results/logs/`
