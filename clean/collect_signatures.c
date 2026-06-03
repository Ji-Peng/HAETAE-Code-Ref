/**
 * HAETAE Signature Collector for Truncation Analysis
 * ===================================================
 * Generates key pair, signs many messages, and exports:
 * - Secret key s1, s2 (int32 coefficients)
 * - For each signature: z1, z2 (polyfix coefficients), challenge c
 *
 * Output format (binary):
 * Header: N, K, L, M, LN, num_signatures (all uint32)
 * Secret key s1: M * N int32 values
 * Secret key s2: K * N int32 values
 * For each signature:
 *   z1: L * N int32 values (polyfix coefficients)
 *   z2: K * N int32 values (polyfix coefficients)
 *   c:  N int32 values (challenge polynomial)
 *   b:  1 uint8 value (bimodal bit)
 */

#include "sign.h"
#include "packing.h"
#include "params.h"
#include "poly.h"
#include "polyfix.h"
#include "polymat.h"
#include "polyvec.h"
#include "randombytes.h"
#include "symmetric.h"
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* We need access to internal signing state.
 * This is a modified version of crypto_sign_signature that exports
 * the internal z1, z2 (polyfix), challenge c, and bimodal bit b.
 */
static int sign_and_export(
    uint8_t *sig, size_t *siglen,
    const uint8_t *m, size_t mlen,
    const uint8_t *sk,
    /* exported internal state */
    polyfixvecl *out_z1,
    polyfixveck *out_z2,
    poly *out_c,
    uint8_t *out_b)
{
    uint8_t buf[POLYVECK_HIGHBITS_PACKEDBYTES + POLYC_PACKEDBYTES] = {0};
    uint8_t seedbuf[CRHBYTES] = {0}, key[SEEDBYTES] = {0};
    uint8_t mu[CRHBYTES] = {0};
    uint8_t b = 0;
    uint16_t counter = 0;
    uint64_t reject1, reject2;

    polyvecm s1;
    polyvecl A1[K], cs1;
    polyveck s2, cs2, highbits, Ay;
    polyfixvecl y1, z1, z1tmp;
    polyfixveck y2, z2, z2tmp;
    polyvecl z1rnd;
    polyvecl hb_z1, lb_z1;
    polyveck z2rnd, h, htmp;
    poly c, chat, z1rnd0, lsb;

    xof256_state state;
    unsigned int i;

    unpack_sk(A1, &s1, &s2, key, sk);

    xof256_absorbe_twice(&state, sk, HAETAE_CRYPTO_PUBLICKEYBYTES, m, mlen);
    xof256_squeeze(mu, CRHBYTES, &state);
    shake256_inc_ctx_release(&state);
    xof256_absorbe_twice(&state, key, SEEDBYTES, mu, CRHBYTES);
    xof256_squeeze(seedbuf, CRHBYTES, &state);
    shake256_inc_ctx_release(&state);

    polyvecm_ntt(&s1);
    polyveck_ntt(&s2);

reject:
    counter = polyfixveclk_sample_hyperball(&y1, &y2, &b, seedbuf, counter);

    polyfixvecl_round(&z1rnd, &y1);
    polyfixveck_round(&z2rnd, &y2);

    z1rnd0 = z1rnd.vec[0];
    polyvecl_ntt(&z1rnd);
    polymatkl_pointwise_montgomery(&Ay, A1, &z1rnd);
    polyveck_invntt_tomont(&Ay);
    polyveck_double(&z2rnd);
    polyveck_add(&Ay, &Ay, &z2rnd);

    polyveck_poly_fromcrt(&Ay, &Ay, &z1rnd0);
    polyveck_freeze2q(&Ay);

    polyveck_highbits_hint(&highbits, &Ay);
    poly_lsb(&lsb, &z1rnd0);

    polyveck_pack_highbits(buf, &highbits);
    poly_pack_lsb(buf + POLYVECK_HIGHBITS_PACKEDBYTES, &lsb);

    poly_challenge(&c, buf, mu);

    cs1.vec[0] = c;
    chat = c;
    poly_ntt(&chat);

    for (i = 1; i < L; ++i) {
        poly_pointwise_montgomery(&cs1.vec[i], &chat, &s1.vec[i - 1]);
        poly_invntt_tomont(&cs1.vec[i]);
    }
    polyveck_poly_pointwise_montgomery(&cs2, &s2, &chat);
    polyveck_invntt_tomont(&cs2);

    polyvecl_cneg(&cs1, b & 1);
    polyveck_cneg(&cs2, b & 1);
    polyfixvecl_add(&z1, &y1, &cs1);
    polyfixveck_add(&z2, &y2, &cs2);

    reject1 = ((uint64_t)B1SQ * LN * LN - polyfixveclk_sqnorm2(&z1, &z2)) >> 63;
    reject1 &= 1;

    polyfixvecl_double(&z1tmp, &z1);
    polyfixveck_double(&z2tmp, &z2);

    polyfixfixvecl_sub(&z1tmp, &z1tmp, &y1);
    polyfixfixveck_sub(&z2tmp, &z2tmp, &y2);

    reject2 =
        (polyfixveclk_sqnorm2(&z1tmp, &z2tmp) - (uint64_t)B0SQ * LN * LN) >> 63;
    reject2 &= 1;
    reject2 &= (b & 0x2) >> 1;

    if (reject1 | reject2) {
        goto reject;
    }

    /* Export internal state */
    *out_z1 = z1;
    *out_z2 = z2;
    *out_c = c;
    *out_b = b;

    /* Still produce the actual signature */
    polyfixvecl_round(&z1rnd, &z1);
    polyfixveck_round(&z2rnd, &z2);

    polyveck_double(&z2rnd);
    polyveck_sub(&htmp, &Ay, &z2rnd);
    polyveck_freeze2q(&htmp);

    polyveck_highbits_hint(&htmp, &htmp);
    polyveck_sub(&h, &highbits, &htmp);
    polyveck_caddDQ2ALPHA(&h);

    polyvecl_lowbits(&lb_z1, &z1rnd);
    polyvecl_highbits(&hb_z1, &z1rnd);

    if (pack_sig(sig, &c, &lb_z1, &hb_z1, &h)) {
        goto reject;
    }
    *siglen = HAETAE_CRYPTO_BYTES;

    return 0;
}

int main(int argc, char *argv[]) {
    int num_sigs = 10000;
    const char *outfile = "signatures.bin";

    if (argc >= 2) num_sigs = atoi(argv[1]);
    if (argc >= 3) outfile = argv[2];

    fprintf(stderr, "HAETAE-%d Signature Collector\n", HAETAE_MODE == 2 ? 120 : (HAETAE_MODE == 3 ? 180 : 260));
    fprintf(stderr, "Collecting %d signatures -> %s\n", num_sigs, outfile);
    fprintf(stderr, "Parameters: N=%d, K=%d, L=%d, M=%d, LN=%d\n", N, K, L, M, LN);

    /* Generate keypair */
    uint8_t pk[HAETAE_CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[HAETAE_CRYPTO_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);

    /* Extract secret key for analysis */
    polyvecm s1_extracted;
    polyveck s2_extracted;
    {
        polyvecl A1_tmp[K];
        uint8_t key_tmp[SEEDBYTES];
        unpack_sk(A1_tmp, &s1_extracted, &s2_extracted, key_tmp, sk);
    }

    /* Open output file */
    FILE *fp = fopen(outfile, "wb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", outfile);
        return 1;
    }

    /* Write header */
    uint32_t header[6] = {N, K, L, M, LN, (uint32_t)num_sigs};
    fwrite(header, sizeof(uint32_t), 6, fp);

    /* Write secret key s1 (M polynomials) */
    for (int i = 0; i < M; i++) {
        fwrite(s1_extracted.vec[i].coeffs, sizeof(int32_t), N, fp);
    }

    /* Write secret key s2 (K polynomials) */
    for (int i = 0; i < K; i++) {
        fwrite(s2_extracted.vec[i].coeffs, sizeof(int32_t), N, fp);
    }

    /* Generate and collect signatures */
    uint8_t sig[HAETAE_CRYPTO_BYTES + 100];
    size_t siglen;
    polyfixvecl z1;
    polyfixveck z2;
    poly c;
    uint8_t b;

    for (int t = 0; t < num_sigs; t++) {
        /* Use counter as message to ensure different signatures */
        uint8_t msg[8];
        memcpy(msg, &t, sizeof(int));
        msg[4] = msg[5] = msg[6] = msg[7] = 0;

        int ret = sign_and_export(sig, &siglen, msg, 8, sk,
                                  &z1, &z2, &c, &b);
        if (ret != 0) {
            fprintf(stderr, "Signing failed at t=%d\n", t);
            continue;
        }

        /* Write z1: L polynomials of polyfix (int32 coefficients) */
        for (int i = 0; i < L; i++) {
            fwrite(z1.vec[i].coeffs, sizeof(int32_t), N, fp);
        }

        /* Write z2: K polynomials of polyfix */
        for (int i = 0; i < K; i++) {
            fwrite(z2.vec[i].coeffs, sizeof(int32_t), N, fp);
        }

        /* Write challenge c */
        fwrite(c.coeffs, sizeof(int32_t), N, fp);

        /* Write bimodal bit b */
        fwrite(&b, sizeof(uint8_t), 1, fp);

        if ((t + 1) % 1000 == 0) {
            fprintf(stderr, "\r  Progress: %d / %d signatures", t + 1, num_sigs);
        }
    }

    fprintf(stderr, "\nDone. Output: %s\n", outfile);
    fclose(fp);
    return 0;
}
