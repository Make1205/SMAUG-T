// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <time.h>

#include "api.h"
#include "cpucycles.h"
#include "params.h"
#include "speed_print.h"

#define NTESTS 10000

uint64_t t[NTESTS];

int main() {

    printf("*** speed %s with mode %s\n", "SMAUGT KEM", CRYPTO_ALGNAME);

    printf("CRYPTO_ALGNAME: %s\n", CRYPTO_ALGNAME);
    printf("CRYPTO_SECRETKEYBYTES: %d\n", CRYPTO_SECRETKEYBYTES);
    printf("CRYPTO_PUBLICKEYBYTES: %d\n", CRYPTO_PUBLICKEYBYTES);
    printf("CRYPTO_CIPHERTEXTBYTES: %d\n", CRYPTO_CIPHERTEXTBYTES);
    printf("CRYPTO_BYTES: %d\n\n", CRYPTO_BYTES);

    uint8_t sk[SMAUGT_KEM_SECRETKEY_BYTES] = {0};
    uint8_t pk[SMAUGT_PUBLICKEY_BYTES] = {0};
    int i = 0;
    clock_t srt, ed;
    clock_t overhead;

    overhead = clock();
    cpucycles();
    overhead = clock() - overhead;

    srt = clock();
    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        crypto_kem_keypair(pk, sk);
    }
    ed = clock();
    print_results("keygen_kem: ", t, NTESTS);
    printf("time elapsed: %.8fms\n\n", (double)(ed - srt - overhead * NTESTS) *
                                           1000 / CLOCKS_PER_SEC / NTESTS);

    uint8_t ctxt[SMAUGT_CIPHERTEXT_BYTES] = {0};
    uint8_t ss1[SMAUGT_CRYPTO_BYTES] = {0};

    srt = clock();
    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        crypto_kem_enc(ctxt, ss1, pk);
    }
    ed = clock();
    print_results("encap: ", t, NTESTS);
    printf("time elapsed: %.8fms\n\n", (double)(ed - srt - overhead * NTESTS) *
                                           1000 / CLOCKS_PER_SEC / NTESTS);

    uint8_t ss2[SMAUGT_CRYPTO_BYTES] = {0};
    srt = clock();
    for (i = 0; i < NTESTS; i++) {
        t[i] = cpucycles();
        crypto_kem_dec(ss2, ctxt, sk);
    }
    ed = clock();
    print_results("decap: ", t, NTESTS);
    printf("time elapsed: %.8fms\n\n", (double)(ed - srt - overhead * NTESTS) *
                                           1000 / CLOCKS_PER_SEC / NTESTS);

    return 0;
}
