// SPDX-License-Identifier: MIT

#ifndef SMAUGT_KAT_API_H
#define SMAUGT_KAT_API_H

#include <stdint.h>

#include "params.h"

#define CRYPTO_PUBLICKEYBYTES SMAUGT_PUBLICKEY_BYTES

// For encrypt
#define CRYPTO_PKE_SECRETKEYBYTES SMAUGT_PKE_SECRETKEY_BYTES

// For kem
#define CRYPTO_KEM_SECRETKEYBYTES SMAUGT_KEM_SECRETKEY_BYTES
#define CRYPTO_CIPHERTEXTBYTES SMAUGT_CIPHERTEXT_BYTES

// For KEM
int kat_crypto_kem_keypair(unsigned char *pk, unsigned char *sk,
                           unsigned char *d, unsigned char *seed);

int kat_crypto_kem_enc(unsigned char *ct, unsigned char *ss, unsigned char *pk,
                       unsigned char *mu);

int kat_crypto_kem_dec(unsigned char *ss, unsigned char *ct, unsigned char *sk);

int crypto_kem_keypair_internal(uint8_t *pk, uint8_t *sk,
                                uint8_t d[SMAUGT_T_BYTES],
                                uint8_t seed[SMAUGT_CRYPTO_BYTES]);
int crypto_kem_keypair(uint8_t *pk, uint8_t *sk);

int crypto_kem_enc_internal(uint8_t *ctxt, uint8_t *ss, const uint8_t *pk,
                            const uint8_t *mu);
int crypto_kem_enc(uint8_t *ctxt, uint8_t *ss, const uint8_t *pk);

int crypto_kem_dec_internal(uint8_t *ss, const uint8_t *ctxt,
                            const uint8_t *sk);
int crypto_kem_dec(uint8_t *ss, const uint8_t *ctxt, const uint8_t *sk);

#endif /* !SMAUGT_KAT_API_H */
