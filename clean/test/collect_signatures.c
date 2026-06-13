/**
 * collect_signatures.c
 *
 * Collects HAETAE signatures with internal state (z1, z2, c, b) for
 * passive key recovery attack analysis.
 *
 * Binary output format:
 *   Header: N(u32) K(u32) L(u32) M(u32) LN(u32) num_sigs(u32)
 *   Secret keys: M polynomials s1 (each N×i32), K polynomials s2 (each N×i32)
 *   For each signature (this build also exports the mask y for analysis):
 *     L polynomials y1, K polynomials y2  (mask, each N×i32, polyfix LN-scaled)
 *     L polynomials z1, K polynomials z2  (response, each N×i32, polyfix LN-scaled)
 *     1 challenge polynomial c (N×i32)
 *     1 byte b (bimodal bits; b&1 = sign, (b>>1)&1 = b')
 *
 * Usage: ./collect_signatures <num_sigs> <output_file>
 *        output_file "-" streams the binary to stdout (logs go to stderr),
 *        so millions of signatures can be piped without a multi-GB disk file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "params.h"
#include "sign.h"
#include "packing.h"
#include "poly.h"
#include "polyfix.h"
#include "polymat.h"
#include "polyvec.h"
#include "randombytes.h"
#include "symmetric.h"

/**
 * Modified signing function that outputs internal state.
 * Returns 0 on success (signature produced), -1 on error.
 * Outputs z1, z2, c, b for the accepted signature.
 */
static int sign_and_collect(
    polyfixvecl *out_z1, polyfixveck *out_z2, poly *out_c, uint8_t *out_b,
    polyfixvecl *out_y1, polyfixveck *out_y2,
    const uint8_t *m, size_t mlen, const uint8_t *sk)
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
    polyveck z2rnd;
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

    reject2 = (polyfixveclk_sqnorm2(&z1tmp, &z2tmp) - (uint64_t)B0SQ * LN * LN) >> 63;
    reject2 &= 1;
    reject2 &= (b & 0x2) >> 1;

    if (reject1 | reject2) {
        goto reject;
    }

    /* Output internal state (mask y AND response z, both polyfix LN-scaled) */
    *out_z1 = z1;
    *out_z2 = z2;
    *out_y1 = y1;
    *out_y2 = y2;
    *out_c = c;
    *out_b = b;

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_sigs> <output_file>\n", argv[0]);
        return 1;
    }

    uint32_t num_sigs = (uint32_t)atoi(argv[1]);
    const char *outfile = argv[2];

    fprintf(stderr, "HAETAE-%d signature collection\n", HAETAE_MODE);
    fprintf(stderr, "  N=%d, K=%d, L=%d, M=%d, LN=%d\n", N, K, L, M, LN);
    fprintf(stderr, "  Collecting %u signatures...\n", num_sigs);

    /* Generate keypair */
    uint8_t pk[HAETAE_CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[HAETAE_CRYPTO_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);

    /* Extract secret key for output */
    polyvecl A1_dummy[K];
    polyvecm s1;
    polyveck s2;
    uint8_t key_dummy[SEEDBYTES];
    unpack_sk(A1_dummy, &s1, &s2, key_dummy, sk);

    /* Open output file */
    /* outfile "-" streams the binary to stdout (for piping into the analyzer
     * without ever creating a multi-GB disk file). All human-readable logs go
     * to stderr in that case so stdout stays a pure binary stream. */
    FILE *fp = (strcmp(outfile, "-") == 0) ? stdout : fopen(outfile, "wb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s for writing\n", outfile);
        return 1;
    }

    /* Write header */
    uint32_t header[6] = {N, K, L, M, LN, num_sigs};
    fwrite(header, sizeof(uint32_t), 6, fp);

    /* Write secret keys: M polynomials of s1, K polynomials of s2 */
    for (unsigned int i = 0; i < M; i++) {
        fwrite(s1.vec[i].coeffs, sizeof(int32_t), N, fp);
    }
    for (unsigned int i = 0; i < K; i++) {
        fwrite(s2.vec[i].coeffs, sizeof(int32_t), N, fp);
    }

    /* Collect signatures.
     * Per-signature record layout (this build, rejection-bias experiment):
     *   y1: L polynomials (N i32, polyfix LN-scaled mask)
     *   y2: K polynomials (N i32)
     *   z1: L polynomials (N i32, polyfix LN-scaled response)
     *   z2: K polynomials (N i32)
     *   c : N i32 challenge
     *   b : 1 byte bimodal bits
     */
    polyfixvecl z1, y1;
    polyfixveck z2, y2;
    poly c;
    uint8_t b;
    uint8_t msg[32];
    clock_t start = clock();

    for (uint32_t t = 0; t < num_sigs; t++) {
        /* Use different message for each signature */
        memcpy(msg, &t, sizeof(t));
        memset(msg + sizeof(t), 0, sizeof(msg) - sizeof(t));

        sign_and_collect(&z1, &z2, &c, &b, &y1, &y2, msg, sizeof(msg), sk);

        /* Write y1 (L polynomials, fixed-point mask) */
        for (unsigned int i = 0; i < L; i++) {
            fwrite(y1.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        /* Write y2 (K polynomials, fixed-point mask) */
        for (unsigned int i = 0; i < K; i++) {
            fwrite(y2.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        /* Write z1 (L polynomials, fixed-point) */
        for (unsigned int i = 0; i < L; i++) {
            fwrite(z1.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        /* Write z2 (K polynomials, fixed-point) */
        for (unsigned int i = 0; i < K; i++) {
            fwrite(z2.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        /* Write challenge c */
        fwrite(c.coeffs, sizeof(int32_t), N, fp);
        /* Write bimodal bit b */
        fwrite(&b, 1, 1, fp);

        if ((t + 1) % 10000 == 0) {
            double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
            double rate = (t + 1) / elapsed;
            double eta = (num_sigs - t - 1) / rate;
            fprintf(stderr, "  [%u/%u] %.0f sigs/sec, ETA: %.0fs\n",
                   t + 1, num_sigs, rate, eta);
        }
    }

    if (fp != stdout) fclose(fp); else fflush(fp);

    double total_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    fprintf(stderr, "Done! %u signatures in %.1fs (%.0f sigs/sec)\n",
           num_sigs, total_time, num_sigs / total_time);
    fprintf(stderr, "Output: %s\n", outfile);

    return 0;
}
