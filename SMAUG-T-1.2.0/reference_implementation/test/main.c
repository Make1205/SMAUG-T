// SPDX-License-Identifier: MIT

#include "api.h"
#include "indcpa.h"
#include "pack.h"
#include "params.h"
#include "poly.h"
#include "randombytes.h"

#include <stdio.h>
#include <string.h>

#define m_size 10

int indcpa_test();
int kem_test();
int packing_test();

int main(void) {
    printf("*** main %s with mode %s\n", "SMAUGT KEM", CRYPTO_ALGNAME);

    printf("CRYPTO_ALGNAME: %s\n", CRYPTO_ALGNAME);
    printf("CRYPTO_SECRETKEYBYTES: %d\n", CRYPTO_SECRETKEYBYTES);
    printf("CRYPTO_PUBLICKEYBYTES: %d\n", CRYPTO_PUBLICKEYBYTES);
    printf("CRYPTO_CIPHERTEXTBYTES: %d\n", CRYPTO_CIPHERTEXTBYTES);
    printf("CRYPTO_BYTES: %d\n\n", CRYPTO_BYTES);

    size_t count = 1;
    const size_t iteration = 100000;

    if (packing_test()) {
        printf("packing_test fail\n");
    }
    for (size_t i = 0; i < iteration; ++i) {
        if (!(i % (iteration / 10))) {
            printf("...%lu%%", count * 10);
            fflush(stdout);
            ++count;
        }

        if (indcpa_test()) {
            printf("\nPKE test fails at %lu-th tries\n", i);
            break;
        }

        if (kem_test()) {
            printf("\nKEM test fails at %lu-th tries\n", i);
            break;
        }
    }
    printf("\n");

    return 0;
}

int check_bytes(uint8_t *bytes1, uint8_t *bytes2, size_t len, char *name) {
    int fail = 0;
    for (size_t i = 0; i < len; i++) {
        if (bytes1[i] != bytes2[i]) {
            fail = 1;
            break;
        }
    }
    if (fail)
        printf("%s fail\n", name);
    else
        printf("%s success\n", name);
    return fail;
}

int check_poly(poly *data1, poly *data2, char *name) {
    int fail = 0;
    for (size_t i = 0; i < SMAUGT_N; i++) {
        if (data1->coeffs[i] != data2->coeffs[i]) {
            fail = 1;
            break;
        }
    }
    if (fail)
        printf("%s fail\n", name);
    else
        printf("%s success\n", name);
    return fail;
}

int packing_test() {
    int fail = 0;
    poly data1, data2;
    uint8_t bytes1[SMAUGT_PKPOLY_BYTES], bytes2[SMAUGT_PKPOLY_BYTES];

    memset(bytes1, 0, SMAUGT_PKPOLY_BYTES);
    memset(bytes2, 0, SMAUGT_PKPOLY_BYTES);
    memset(&data1, 0, sizeof(poly));
    memset(&data2, 0, sizeof(poly));
    randombytes(bytes1, SMAUGT_PKPOLY_BYTES);
    memset(bytes1, 0, SMAUGT_PKPOLY_BYTES);
    pack_ring(bytes1, &data1);

    unpack_ring(&data2, bytes1);
    pack_ring(bytes2, &data2);
    printf("SMAUGT_LOG_Q: %d\n", SMAUGT_LOG_Q);
    fail += check_bytes(bytes1, bytes2, SMAUGT_PKPOLY_BYTES, "unpack_ring");
    fail += check_poly(&data1, &data2, "pack_ring");

    memset(bytes1, 0, SMAUGT_PKPOLY_BYTES);
    memset(bytes2, 0, SMAUGT_PKPOLY_BYTES);
    memset(&data1, 0, sizeof(poly));
    memset(&data2, 0, sizeof(poly));
    randombytes(bytes1, SMAUGT_CTPOLY1_BYTES);
    unpack_ring_p(&data1, bytes1);
    pack_ring_p(bytes1, &data1);

    unpack_ring_p(&data2, bytes1);
    pack_ring_p(bytes2, &data2);

    printf("SMAUGT_LOG_P: %d\n", SMAUGT_LOG_P);
    fail += check_bytes(bytes1, bytes2, SMAUGT_CTPOLY1_BYTES, "unpack_ring_p");
    fail += check_poly(&data1, &data2, "pack_ring_p");

    memset(bytes1, 0, SMAUGT_PKPOLY_BYTES);
    memset(bytes2, 0, SMAUGT_PKPOLY_BYTES);
    memset(&data1, 0, sizeof(poly));
    memset(&data2, 0, sizeof(poly));
    randombytes(bytes1, SMAUGT_CTPOLY2_BYTES);
    unpack_ring_p_prime(&data1, bytes1);
    pack_ring_p_prime(bytes1, &data1);

    unpack_ring_p_prime(&data2, bytes1);
    pack_ring_p_prime(bytes2, &data2);

    printf("SMAUGT_LOG_P_PRIME: %d\n", SMAUGT_LOG_P_PRIME);
    fail += check_bytes(bytes1, bytes2, SMAUGT_CTPOLY2_BYTES,
                        "unpack_ring_p_prime");
    fail += check_poly(&data1, &data2, "pack_ring_p_prime");

    return fail;
}

int indcpa_test() {
    uint8_t pk[SMAUGT_PUBLICKEY_BYTES] = {0};
    uint8_t sk[SMAUGT_PKE_SECRETKEY_BYTES] = {0};
    uint8_t ctxt[SMAUGT_CIPHERTEXT_BYTES] = {0};
    uint8_t mu[SMAUGT_MSG_BYTES] = {0}, mu2[SMAUGT_MSG_BYTES] = {0};
    uint8_t seed[SMAUGT_CRYPTO_BYTES] = {0};

    randombytes(seed, SMAUGT_CRYPTO_BYTES);
    indcpa_keypair(pk, sk, seed);

    randombytes(mu, SMAUGT_MSG_BYTES);
    indcpa_enc(ctxt, pk, mu, seed);

    indcpa_dec(mu2, sk, ctxt);

    if (memcmp(mu, mu2, SMAUGT_MSG_BYTES) != 0) {
        printf("\n");
        for (int i = 0; i < m_size; ++i)
            printf("0x%2hx ", mu[i]);
        printf("\n");

        for (int i = 0; i < m_size; ++i)
            printf("0x%2hx ", mu2[i]);
        printf("\n");
        return 1;
    }

    return 0;
}

int kem_test() {
    uint8_t pk[SMAUGT_PUBLICKEY_BYTES] = {0};
    uint8_t sk[SMAUGT_KEM_SECRETKEY_BYTES] = {0};

    crypto_kem_keypair(pk, sk);

    uint8_t ctxt[SMAUGT_CIPHERTEXT_BYTES] = {0};
    uint8_t ss[SMAUGT_CRYPTO_BYTES] = {0}, ss2[SMAUGT_CRYPTO_BYTES] = {0};
    crypto_kem_enc(ctxt, ss, pk);

    int res = crypto_kem_dec(ss2, ctxt, sk);

    if (memcmp(ss, ss2, SMAUGT_CRYPTO_BYTES) != 0) {
        printf("\n");
        for (int i = 0; i < m_size; ++i) {
            printf("0x%2hx ", ss[i]);
        }
        printf("\n");

        for (int i = 0; i < m_size; ++i) {
            printf("0x%2hx ", ss2[i]);
        }
        printf("\n");
    }

    return res;
}
