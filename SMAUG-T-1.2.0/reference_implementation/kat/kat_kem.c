// SPDX-License-Identifier: MIT

#include "api.h"
#include "kat_api.h"

int kat_crypto_kem_keypair(unsigned char *pk, unsigned char *sk,
                           unsigned char *d, unsigned char *seed) {
    crypto_kem_keypair_internal(pk, sk, d, seed);
    return 0;
}

int kat_crypto_kem_enc(unsigned char *ct, unsigned char *ss, unsigned char *pk,
                       unsigned char *mu) {
    return crypto_kem_enc_internal(ct, ss, pk, mu);
}

int kat_crypto_kem_dec(unsigned char *ss, unsigned char *ct,
                       unsigned char *sk) {
    return crypto_kem_dec(ss, ct, sk);
}
