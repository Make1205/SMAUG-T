// SPDX-License-Identifier: MIT

#include "gtest/gtest.h"

extern "C" {
#include "api.h"
#include "cbd.h"
#include "ciphertext.h"
#include "fips202.h"
#include "indcpa.h"
#include "pack.h"
#include "params.h"
#include "poly.h"
#include "randombytes.h"
}

template <class Ty>
static bool arrayEq(const Ty *a, const Ty *b, unsigned len) {
    for (unsigned i = 0; i < len; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

static bool polyEq(const poly &a, const poly &b, unsigned len) {
    for (unsigned i = 0; i < len; ++i) {
        if (a.coeffs[i] != b.coeffs[i])
            return false;
    }
    return true;
}

bool RvecEq(const polyvec &a, const polyvec &b) {
    for (size_t i = 0; i < SMAUGT_K; ++i) {
        if (!polyEq(a.vec[i], b.vec[i], SMAUGT_N))
            return false;
    }
    return true;
}

bool RmatEq(const polyvec a[SMAUGT_K], const polyvec b[SMAUGT_K]) {
    for (size_t i = 0; i < SMAUGT_K; ++i) {
        if (!RvecEq(a[i], b[i]))
            return false;
    }
    return true;
}

bool SxvecEq(const uint8_t *a[SMAUGT_K], const uint8_t *b[SMAUGT_K],
             const uint8_t cnt_arr[SMAUGT_K]) {
    for (size_t i = 0; i < SMAUGT_K; ++i) {
        if (!arrayEq(a[i], b[i], cnt_arr[i]))
            return false;
    }
    return true;
}

void checkPkEq(const uint8_t pk1[SMAUGT_PUBLICKEY_BYTES],
               const uint8_t pk2[SMAUGT_PUBLICKEY_BYTES]) {
    ASSERT_FALSE(memcmp(pk1, pk2, SMAUGT_PUBLICKEY_BYTES))
        << "Public keys must be equal";
}

bool isPkDiff(const public_key &pk1, const public_key &pk2) {
    return !RmatEq(pk1.A, pk2.A) || !RvecEq(pk1.b, pk2.b);
}

void checkSkEq(const uint8_t sk1[SMAUGT_KEM_SECRETKEY_BYTES],
               const uint8_t sk2[SMAUGT_KEM_SECRETKEY_BYTES], bool isPKE) {
    if (!isPKE) {
        ASSERT_FALSE(memcmp(sk1, sk2, SMAUGT_PKE_SECRETKEY_BYTES))
            << "Secret keys of PKE must be equal";
    } else {
        ASSERT_FALSE(memcmp(sk1, sk2, SMAUGT_KEM_SECRETKEY_BYTES))
            << "Secret keys of KEM must be equal";
    }
}

bool isSkDiff(const secret_key &sk1, const secret_key &sk2) {
    bool res = false;
    for (size_t i = 0; i < SMAUGT_K; ++i)
        res |= !arrayEq(sk1.vec[i].coeffs, sk2.vec[i].coeffs, SMAUGT_N);

    return res;
}

void checkCtxtEq(const uint8_t ctxt1[SMAUGT_CIPHERTEXT_BYTES],
                 const uint8_t ctxt2[SMAUGT_CIPHERTEXT_BYTES]) {
    ASSERT_FALSE(memcmp(ctxt1, ctxt2, SMAUGT_CIPHERTEXT_BYTES))
        << "Ciphertexts are different";
}

bool isCtxtDiff(const ciphertext &ctxt1, const ciphertext &ctxt2, bool isPKE) {
    return !RvecEq(ctxt1.c1, ctxt2.c1) ||
           !arrayEq(ctxt1.c2.coeffs, ctxt2.c2.coeffs, SMAUGT_N);
}

void testPacking() {
    const unsigned count = 10000;
    for (size_t i = 0; i < count; i++) {
        uint8_t bytes1[SMAUGT_PKPOLYMAT_BYTES];
        uint8_t bytes2[SMAUGT_PKPOLYMAT_BYTES];
        polyvec data[SMAUGT_K] = {0};

        randombytes(bytes1, SMAUGT_PKPOLYMAT_BYTES);
        memcpy(bytes2, bytes1, SMAUGT_PKPOLYMAT_BYTES);
        unpack_ring_mat(data, bytes1);
        pack_ring_mat(bytes1, data);
        ASSERT_TRUE(arrayEq(bytes1, bytes2, SMAUGT_PKPOLYMAT_BYTES));
    }
    for (size_t i = 0; i < count; i++) {
        uint8_t bytes1[SMAUGT_CTPOLYVEC_BYTES];
        uint8_t bytes2[SMAUGT_CTPOLYVEC_BYTES];
        polyvec data;

        randombytes(bytes1, SMAUGT_CTPOLYVEC_BYTES);
        memcpy(bytes2, bytes1, SMAUGT_CTPOLYVEC_BYTES);
        unpack_ring_p_vec(&data, bytes1);
        pack_ring_p_vec(bytes1, &data);
        ASSERT_TRUE(arrayEq(bytes1, bytes2, SMAUGT_CTPOLYVEC_BYTES));
    }
    for (size_t i = 0; i < count; i++) {
        uint8_t bytes[SMAUGT_SKPOLYVEC_BYTES] = {0};
        secret_key s1, s2;

        uint8_t seed[SMAUGT_CRYPTO_BYTES] = {0};
        randombytes(seed, SMAUGT_CRYPTO_BYTES);
        expand_s(&s1, seed);

        for (size_t j = 0; j < SMAUGT_K; ++j) {
            pack_s_poly(bytes, &s1.vec[j]);
            unpack_s_poly(&s2.vec[j], bytes);

            ASSERT_TRUE(arrayEq(s1.vec[j].coeffs, s2.vec[j].coeffs, SMAUGT_N));
        }
    }
}

static inline int weight(int *plus, int *minus, const int16_t p[SMAUGT_N]) {
    int zero = 0;
    for (size_t i = 0; i < SMAUGT_N; i++) {
        if (p[i] == 1)
            *plus += 1;
        else if (p[i] == -1)
            *minus += 1;
        else if (p[i] == 0)
            zero += 1;
        else
            return p[i];
    }
    return zero;
}

void testCBD() {
    const unsigned count = 10000;
    for (size_t i = 0; i < count; i++) {
        poly r;
        int plus = 0, minus = 0;
        uint8_t seed[SMAUGT_CRYPTO_BYTES] = {0};
#if defined(AVX_KECCAK) || defined(AVX_90S)
        ALIGNED_UINT8(SMAUGT_CBDSEED_BYTES) extseed;
#else
        uint8_t extseed[SMAUGT_CBDSEED_BYTES];
#endif
        randombytes(seed, SMAUGT_CRYPTO_BYTES);

#if defined(AVX_KECCAK)
        shake256(extseed.coeffs, SMAUGT_CBDSEED_BYTES, seed,
                 SMAUGT_CRYPTO_BYTES);
        sp_cbd(&r, extseed.CBDSEED_FIELD);
#elif defined(AVX_90S)
        prf(extseed.coeffs, SMAUGT_CBDSEED_BYTES, seed, 0);
        sp_cbd(&r, extseed.CBDSEED_FIELD);
#else
        shake256(extseed, SMAUGT_CBDSEED_BYTES, seed, SMAUGT_CRYPTO_BYTES);
        sp_cbd(&r, extseed);
#endif

        int zero = weight(&plus, &minus, r.coeffs);
        ASSERT_TRUE((zero + plus + minus) == SMAUGT_N);
    }
}

void testMultOneVector() {
    const unsigned count = 10000;
#if defined(AVX_KECCAK) || defined(AVX_90S)
    nttpolyvec shat[2];
#endif
    for (size_t i = 0; i < count; i++) { // check A * [1] ^ (SMAUGT_K)
        uint8_t matbytes[SMAUGT_PKPOLYMAT_BYTES];
        polyvec A[SMAUGT_K];
        polyvec vec1, vec2;
        polyvec s;
        memset(&s, 0, sizeof s);

        randombytes(matbytes, SMAUGT_PKPOLYMAT_BYTES);
        unpack_ring_mat(A, matbytes);
        memset(&vec1, 0, sizeof vec1);
        memset(&vec2, 0, sizeof vec2);
        for (size_t j = 0; j < SMAUGT_K; j++) {
            s.vec[j].coeffs[0] = 1;
            for (size_t k = 0; k < SMAUGT_K; k++) {
                for (size_t l = 0; l < SMAUGT_N; l++) {
                    vec2.vec[k].coeffs[l] += A[j].vec[k].coeffs[l];
                }
            }
        }
#if defined(AVX_KECCAK) || defined(AVX_90S)
        matrix_vec_mult_add(&vec1, shat, A, &s);
#else
        matrix_vec_mult_add(&vec1, A, &s);
#endif
        ASSERT_TRUE(RvecEq(vec1, vec2));
    }
    for (size_t i = 0; i < count; i++) { // check -(A * -[1] ^ (SMAUGT_K))
        uint8_t matbytes[SMAUGT_PKPOLYMAT_BYTES];
        polyvec A[SMAUGT_K];
        polyvec vec1, vec2;
        polyvec s;
        memset(&s, 0, sizeof s);

        randombytes(matbytes, SMAUGT_PKPOLYMAT_BYTES);
        unpack_ring_mat(A, matbytes);
        memset(&vec1, 0, sizeof vec1);
        memset(&vec2, 0, sizeof vec2);
        for (size_t j = 0; j < SMAUGT_K; j++) {
            s.vec[j].coeffs[0] = -1;
            for (size_t k = 0; k < SMAUGT_K; k++) {
                for (size_t l = 0; l < SMAUGT_N; l++) {
                    vec2.vec[j].coeffs[l] += A[j].vec[k].coeffs[l];
                }
            }
        }
        matrix_vec_mult_sub(&vec1, A, &s);
        ASSERT_TRUE(RvecEq(vec1, vec2));
    }
    for (size_t i = 0; i < count; i++) { // check b^(T) * [1] ^ (SMAUGT_K)
        polyvec vec1;
        polyvec s;
        poly res;
        poly sum;
        memset(&s, 0, sizeof s);

        uint8_t vecbytes[SMAUGT_PKPOLYVEC_BYTES];
        randombytes(vecbytes, SMAUGT_PKPOLYVEC_BYTES);
        unpack_ring_vec(&vec1, vecbytes);
        memset(&res, 0, sizeof res);
        memset(&sum, 0, sizeof sum);
        for (size_t j = 0; j < SMAUGT_K; j++) {
            s.vec[j].coeffs[0] = 1;
            for (size_t k = 0; k < SMAUGT_N; k++) {
                sum.coeffs[k] += vec1.vec[j].coeffs[k];
            }
        }
#if defined(AVX_KECCAK) || defined(AVX_90S)
        polyvec_ntt(&shat[0], &s, PDATA0);
        polyvec_ntt(&shat[1], &s, PDATA1);
        vec_vec_mult_add_q(&res, &vec1, shat);
#else
        vec_vec_mult_add(&res, &vec1, &s, SMAUGT_MODULUS_16_LOG_Q);
#endif
        ASSERT_TRUE(polyEq(res, sum, SMAUGT_N));
    }
}

void testMultAddSub() {
    const unsigned count = 10000;
    for (size_t i = 0; i < count; i++) {
        uint8_t matbytes[SMAUGT_PKPOLYMAT_BYTES];
        polyvec A[SMAUGT_K], At[SMAUGT_K];
        uint8_t vecbytes[SMAUGT_PKPOLYVEC_BYTES];
        polyvec vec1, vec2;
        uint8_t sk_seed[SMAUGT_CRYPTO_BYTES] = {0};
        secret_key sk;
        randombytes(matbytes, SMAUGT_PKPOLYMAT_BYTES);
        randombytes(vecbytes, SMAUGT_PKPOLYVEC_BYTES);
        randombytes(sk_seed, SMAUGT_CRYPTO_BYTES);
        unpack_ring_mat(A, matbytes);
        unpack_ring_vec(&vec1, vecbytes);
        unpack_ring_vec(&vec2, vecbytes);
        expand_s(&sk, sk_seed);
        for (size_t l = 0; l < SMAUGT_K; ++l) {
            for (size_t j = 0; j < SMAUGT_K; ++j)
                for (size_t k = 0; k < SMAUGT_N; ++k)
                    At[l].vec[j].coeffs[k] = A[j].vec[l].coeffs[k];
        }

        polyvec res;
        memset(&res, 0, sizeof res);
#if defined(AVX_KECCAK) || defined(AVX_90S)
        nttpolyvec shat;
        matrix_vec_mult_add(&res, &shat, A, &sk);
#else
        matrix_vec_mult_add(&res, A, &sk);
#endif
        for (size_t l = 0; l < SMAUGT_K; ++l) {
            for (size_t k = 0; k < SMAUGT_N; ++k)
                vec1.vec[l].coeffs[k] += res.vec[l].coeffs[k];
        }
        matrix_vec_mult_sub(&vec1, At, &sk);
        ASSERT_TRUE(RvecEq(vec1, vec2));
    }
}

// Test storing + loading random public and secret keys
void testKeyLoadStore(bool isPKE) {
    const unsigned count = 10000;

    for (unsigned i = 0; i < count; ++i) {
        uint8_t sk[SMAUGT_KEM_SECRETKEY_BYTES] = {0};
        uint8_t pk[SMAUGT_PUBLICKEY_BYTES] = {0};
        if (isPKE) {
            uint8_t seed[SMAUGT_CRYPTO_BYTES] = {0};
            randombytes(seed, SMAUGT_CRYPTO_BYTES);
            indcpa_keypair(pk, sk, seed);
        } else {
            crypto_kem_keypair(pk, sk);
        }

        uint8_t pk2[SMAUGT_PUBLICKEY_BYTES] = {0};
        public_key pk_tmp;
        unpack_enck(&pk_tmp, pk);
        pack_enck(pk2, &pk_tmp);
        checkPkEq(pk, pk2);

        uint8_t sk2[SMAUGT_KEM_SECRETKEY_BYTES] = {0};
        secret_key sk_tmp;
        unpack_deck(&sk_tmp, sk);
        pack_deck(sk2, &sk_tmp);
        memcpy(sk2 + SMAUGT_PKE_SECRETKEY_BYTES,
               sk + SMAUGT_PKE_SECRETKEY_BYTES, SMAUGT_T_BYTES);
        checkSkEq(sk, sk2, isPKE);
    }
}

// Test storing + loading random ciphertexts (shared secrets)
void testCiphertextLoadStore(bool isPKE) {
    const unsigned count = 10000;

    for (unsigned i = 0; i < count; ++i) {
        uint8_t sk[SMAUGT_KEM_SECRETKEY_BYTES] = {0};
        uint8_t pk[SMAUGT_PUBLICKEY_BYTES] = {0};
        if (isPKE) {
            uint8_t seed[SMAUGT_CRYPTO_BYTES] = {0};
            randombytes(seed, SMAUGT_CRYPTO_BYTES);
            indcpa_keypair(pk, sk, seed);
        } else {
            crypto_kem_keypair(pk, sk);
        }

        uint8_t mu[SMAUGT_CRYPTO_BYTES] = {0}, seed_r[SMAUGT_DELTA_BYTES] = {0};
        randombytes(mu, SMAUGT_CRYPTO_BYTES);
        randombytes(seed_r, SMAUGT_DELTA_BYTES);

        uint8_t ctxt[SMAUGT_CIPHERTEXT_BYTES] = {0};
        if (isPKE)
            indcpa_enc(ctxt, pk, mu, seed_r);
        else {
            int encryptRes = crypto_kem_enc(ctxt, mu, pk);
            ASSERT_EQ(encryptRes, 0) << "crypto_kem_encapsulation failed";
        }

        uint8_t ctxt2[SMAUGT_CIPHERTEXT_BYTES] = {0};
        ciphertext ctxt_tmp;
        unpack_ct(&ctxt_tmp, ctxt);
        pack_ct(ctxt2, &ctxt_tmp);

        checkCtxtEq(ctxt, ctxt2);
    }
}

// Test encoding + decoding random plaintexts
void testCiphertextEncDec(bool isPKE) {
    const unsigned count = 10000;

    for (unsigned i = 0; i < count; ++i) {
        uint8_t sk[SMAUGT_KEM_SECRETKEY_BYTES] = {0};
        uint8_t pk[SMAUGT_PUBLICKEY_BYTES] = {0};
        if (isPKE) {
            uint8_t seed[SMAUGT_CRYPTO_BYTES] = {0};
            randombytes(seed, SMAUGT_CRYPTO_BYTES);
            indcpa_keypair(pk, sk, seed);
        } else {
            crypto_kem_keypair(pk, sk);
        }

        if (isPKE) {
            uint8_t mx[SMAUGT_MSG_BYTES] = {0}, seed[SMAUGT_DELTA_BYTES] = {0};
            uint8_t ctxt[SMAUGT_CIPHERTEXT_BYTES] = {0};
            uint8_t mx2[SMAUGT_MSG_BYTES] = {0};

            randombytes(mx, SMAUGT_MSG_BYTES);
            randombytes(seed, SMAUGT_DELTA_BYTES);

            indcpa_enc(ctxt, pk, mx, seed);
            indcpa_dec(mx2, sk, ctxt);

            ASSERT_TRUE(arrayEq(mx, mx2, SMAUGT_MSG_BYTES))
                << "Messages are different";

        } else {
            uint8_t mx[SMAUGT_CRYPTO_BYTES] = {0},
                    seed[SMAUGT_DELTA_BYTES] = {0};
            uint8_t ctxt[SMAUGT_CIPHERTEXT_BYTES] = {0};
            uint8_t mx2[SMAUGT_CRYPTO_BYTES] = {0};

            randombytes(mx, SMAUGT_CRYPTO_BYTES);
            randombytes(seed, SMAUGT_DELTA_BYTES);

            int encapRes = crypto_kem_enc(ctxt, mx, pk);
            ASSERT_EQ(encapRes, 0) << "crypto_kem_encapsulation failed";

            int decapRes = crypto_kem_dec(mx2, ctxt, sk);
            ASSERT_EQ(decapRes, 0) << "crypto_kem_decapsulation failed";

            ASSERT_TRUE(arrayEq(mx, mx2, SMAUGT_MSG_BYTES))
                << "Messages are different";
        }
    }
}

TEST(General, Packing) { testPacking(); }
TEST(General, CBD) { testCBD(); }
TEST(General, MultOneVector) { testMultOneVector(); }
TEST(General, MultAddSub) { testMultAddSub(); }

TEST(PKE, EncDec) { testCiphertextEncDec(true); }
TEST(PKE, KeyLoadStore) { testKeyLoadStore(true); }
TEST(PKE, CiphertextLoadStore) { testCiphertextLoadStore(true); }

TEST(KEM, EncDec) { testCiphertextEncDec(false); }
TEST(KEM, KeyLoadStore) { testKeyLoadStore(false); }
TEST(KEM, CiphertextLoadStore) { testCiphertextLoadStore(false); }
