// Local SMAUG-T benchmark harness. This file intentionally lives outside the
// upstream SMAUG-T source tree and only calls the public KEM API.
#include "api.h"
#include "params.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(__i386__)
static inline uint64_t smaugt_cycles(void) {
    uint32_t lo, hi;
    __asm__ __volatile__("lfence\n\trdtsc\n\t" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}
static const char *smaugt_timing_method(void) { return "rdtsc_lfence_median"; }
#else
#include <time.h>
static inline uint64_t smaugt_cycles(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}
static const char *smaugt_timing_method(void) { return "wall_clock_median"; }
#endif

static int cmp_u64(const void *a, const void *b) {
    const uint64_t x = *(const uint64_t *)a;
    const uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static uint64_t median_u64(uint64_t *v, size_t n) {
    qsort(v, n, sizeof(v[0]), cmp_u64);
    if ((n & 1U) != 0U) {
        return v[n / 2U];
    }
    return (v[n / 2U - 1U] / 2U) + (v[n / 2U] / 2U) +
           ((v[n / 2U - 1U] & v[n / 2U] & 1U) != 0U);
}

static int correctness_once(void) {
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_enc[CRYPTO_BYTES];
    uint8_t ss_dec[CRYPTO_BYTES];

    if (crypto_kem_keypair(pk, sk) != 0) {
        fprintf(stderr, "crypto_kem_keypair failed\n");
        return 1;
    }
    if (crypto_kem_enc(ct, ss_enc, pk) != 0) {
        fprintf(stderr, "crypto_kem_enc failed\n");
        return 1;
    }
    if (crypto_kem_dec(ss_dec, ct, sk) != 0) {
        fprintf(stderr, "crypto_kem_dec failed\n");
        return 1;
    }
    if (memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
        fprintf(stderr, "shared-secret mismatch\n");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    size_t iterations = 1000;
    if (argc > 1) {
        char *end = NULL;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || parsed == 0UL) {
            fprintf(stderr, "invalid iteration count: %s\n", argv[1]);
            return 2;
        }
        iterations = (size_t)parsed;
    }

    if (correctness_once() != 0) {
        return 3;
    }

    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_enc[CRYPTO_BYTES];
    uint8_t ss_dec[CRYPTO_BYTES];

    if (crypto_kem_keypair(pk, sk) != 0 || crypto_kem_enc(ct, ss_enc, pk) != 0 ||
        crypto_kem_dec(ss_dec, ct, sk) != 0 || memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
        fprintf(stderr, "benchmark setup correctness failed\n");
        return 4;
    }

    uint64_t *kg = calloc(iterations, sizeof(*kg));
    uint64_t *enc = calloc(iterations, sizeof(*enc));
    uint64_t *dec = calloc(iterations, sizeof(*dec));
    if (kg == NULL || enc == NULL || dec == NULL) {
        fprintf(stderr, "allocation failed\n");
        free(kg);
        free(enc);
        free(dec);
        return 5;
    }

    for (size_t i = 0; i < iterations; ++i) {
        uint64_t t0 = smaugt_cycles();
        int rc = crypto_kem_keypair(pk, sk);
        uint64_t t1 = smaugt_cycles();
        if (rc != 0) {
            fprintf(stderr, "crypto_kem_keypair failed during benchmark\n");
            free(kg); free(enc); free(dec);
            return 6;
        }
        kg[i] = t1 - t0;
    }

    if (crypto_kem_keypair(pk, sk) != 0) {
        free(kg); free(enc); free(dec);
        return 7;
    }
    for (size_t i = 0; i < iterations; ++i) {
        uint64_t t0 = smaugt_cycles();
        int rc = crypto_kem_enc(ct, ss_enc, pk);
        uint64_t t1 = smaugt_cycles();
        if (rc != 0) {
            fprintf(stderr, "crypto_kem_enc failed during benchmark\n");
            free(kg); free(enc); free(dec);
            return 8;
        }
        enc[i] = t1 - t0;
    }

    if (crypto_kem_enc(ct, ss_enc, pk) != 0) {
        free(kg); free(enc); free(dec);
        return 9;
    }
    for (size_t i = 0; i < iterations; ++i) {
        uint64_t t0 = smaugt_cycles();
        int rc = crypto_kem_dec(ss_dec, ct, sk);
        uint64_t t1 = smaugt_cycles();
        if (rc != 0 || memcmp(ss_enc, ss_dec, CRYPTO_BYTES) != 0) {
            fprintf(stderr, "crypto_kem_dec failed during benchmark\n");
            free(kg); free(enc); free(dec);
            return 10;
        }
        dec[i] = t1 - t0;
    }

    const uint64_t kg_med = median_u64(kg, iterations);
    const uint64_t enc_med = median_u64(enc, iterations);
    const uint64_t dec_med = median_u64(dec, iterations);

    printf("alg=%s\n", CRYPTO_ALGNAME);
    printf("q=%u\n", (unsigned)(1U << SMAUGT_LOG_Q));
    printf("pk_bytes=%u\n", (unsigned)CRYPTO_PUBLICKEYBYTES);
    printf("ct_bytes=%u\n", (unsigned)CRYPTO_CIPHERTEXTBYTES);
    printf("sk_bytes=%u\n", (unsigned)CRYPTO_SECRETKEYBYTES);
    printf("ss_bytes=%u\n", (unsigned)CRYPTO_BYTES);
    printf("iterations=%zu\n", iterations);
    printf("KG_kCycles=%.3f\n", (double)kg_med / 1000.0);
    printf("Enc_kCycles=%.3f\n", (double)enc_med / 1000.0);
    printf("Dec_kCycles=%.3f\n", (double)dec_med / 1000.0);
    printf("timing_method=%s\n", smaugt_timing_method());
    printf("correctness_status=pass\n");

    free(kg);
    free(enc);
    free(dec);
    return 0;
}
