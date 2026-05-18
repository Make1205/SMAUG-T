// SPDX-License-Identifier: MIT

#include <stdint.h>
#include <stdio.h>

#include "api.h"
#include "params.h"

#define TEST_LOOP 10000

uint64_t t[TEST_LOOP];

int64_t cpucycles(void) {
#if defined(__x86_64__)
    unsigned hi, lo;

    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));

    return ((uint64_t)hi << 32) | ((int64_t)lo);
#elif defined(__aarch64__)
    uint64_t val;

    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));

    return val;
#else
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#endif
}

int PQC_bench(void) {
    unsigned char pk
        [SMAUGT_PUBLICKEY_BYTES]; // CRYPTO_PUBLICKEYBYTES->SMAUGT_PUBLICKEY_BYTES
    unsigned char sk
        [SMAUGT_KEM_SECRETKEY_BYTES]; // CRYPTO_SECRETKEYBYTES->SMAUGT_KEM_SECRETKEY_BYTES
    unsigned char ctxt
        [SMAUGT_CIPHERTEXT_BYTES]; // ct->ctxt,
                                   // CRYPTO_CIPHERTEXTBYTES->SMAUGT_CIPHERTEXT_BYTES
    unsigned char ss1[SMAUGT_CRYPTO_BYTES]; // ss->ss1
    unsigned char ss2[SMAUGT_CRYPTO_BYTES]; // dss->ss2

    unsigned long long kcycles;
    unsigned long long cycles1, cycles2;

    printf("*** benchmark %s with mode %s\n", "SMAUGT KEM", CRYPTO_ALGNAME);

    printf("CRYPTO_ALGNAME: %s\n", CRYPTO_ALGNAME);
    printf("CRYPTO_SECRETKEYBYTES: %d\n", CRYPTO_SECRETKEYBYTES);
    printf("CRYPTO_PUBLICKEYBYTES: %d\n", CRYPTO_PUBLICKEYBYTES);
    printf("CRYPTO_CIPHERTEXTBYTES: %d\n", CRYPTO_CIPHERTEXTBYTES);
    printf("CRYPTO_BYTES: %d\n\n", CRYPTO_BYTES);

    printf("Number of loop: %d \n", TEST_LOOP);
    printf("KeyGen ////////////////////////////////////////////// \n");

    kcycles = 0;
    for (int i = 0; i < TEST_LOOP; i++) {
        cycles1 = cpucycles();
        crypto_kem_keypair(pk, sk);
        cycles2 = cpucycles();
        kcycles += cycles2 - cycles1;
    }
    printf("  KeyGen runs in ................. %8lld cycles",
           kcycles / TEST_LOOP);
    printf("\n");

    printf("Encapsulation /////////////////////////////////////// \n");
    kcycles = 0;
    for (int i = 0; i < TEST_LOOP; i++) {
        cycles1 = cpucycles();
        crypto_kem_enc(ctxt, ss1, pk);
        cycles2 = cpucycles();
        kcycles += cycles2 - cycles1;
    }

    printf("  Encapsulation  runs in ......... %8lld cycles",
           kcycles / TEST_LOOP);
    printf("\n");

    printf("Decapsulation /////////////////////////////////////// \n");
    kcycles = 0;
    for (int i = 0; i < TEST_LOOP; i++) {
        cycles1 = cpucycles();
        crypto_kem_dec(ss2, ctxt, sk);
        cycles2 = cpucycles();
        kcycles += cycles2 - cycles1;
    }

    printf("  Decapsulation  runs in ......... %8lld cycles",
           kcycles / TEST_LOOP);
    printf("\n");

    printf("==================================================== \n");

    return 0;
}

int main(int argc, char const *argv[]) {
    PQC_bench();

    return 0;
}
