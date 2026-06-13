/**
 * collect_hint_view.c
 *
 * Collects HAETAE signatures with verifier hint internals.  This is a
 * diagnostic collector for passive-attack modeling: it exports
 * packed-signature hint fields and verifier reconstruction intermediates
 * while keeping the old public-view collector format unchanged.
 *
 * Binary output format:
 *   Header: magic(u32='HHV1') N K L M LN num_sigs rec_i32
 *   Secret keys: M polynomials s1, K polynomials s2 (i32, for scoring
 * only) For each signature: signer z1rnd : L polynomials (rounded integer
 * response) signer z2rnd : K polynomials (rounded integer response)
 *     verifier z2  : K polynomials (reconstructed \tilde{z}_2)
 *     challenge c  : N coefficients
 *     hint h       : K polynomials from the packed signature
 *     hint base w0 : K polynomials, highbits_hint(raw verifier value)
 *     hint used w1 : K polynomials, w0+h reduced modulo hint range
 *     raw value    : K polynomials before highbits_hint in verification
 *     b            : one int32 diagnostic branch byte (not public)
 *
 * Usage: ./collect_hint2 <num_sigs> <output_file>
 *        output_file "-" streams binary to stdout; logs go to stderr.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "packing.h"
#include "params.h"
#include "poly.h"
#include "polyfix.h"
#include "polymat.h"
#include "polyvec.h"
#include "randombytes.h"
#include "sign.h"
#include "symmetric.h"

#define HINT_VIEW_MAGIC 0x31564848u

static void reconstruct_hint_response(polyvecl *out_z1, polyveck *out_z2,
                                      polyveck *out_w0, polyveck *out_w1,
                                      polyveck *out_raw, const uint8_t *pk,
                                      const poly *c,
                                      const polyvecl *lowbits_z1,
                                      const polyvecl *highbits_z1,
                                      const polyveck *h)
{
    unsigned int i;
    uint8_t rhoprime[SEEDBYTES] = {0};
    polyvecl A1[K], z1tmp;
    polyveck b, highbits;
#if D > 0
    polyveck a;
#endif
    poly wprime;

    unpack_pk(&b, rhoprime, pk);

    for (i = 0; i < L; ++i) {
        poly_compose(&out_z1->vec[i], &highbits_z1->vec[i],
                     &lowbits_z1->vec[i]);
    }
    z1tmp = *out_z1;

    polymatkl_expand(A1, rhoprime);
    polymatkl_double(A1);
#if D == 1
    polyveck_expand(&a, rhoprime);
    polyveck_double(&b);
    polyveck_sub(&b, &a, &b);
    polyveck_double(&b);
    polyveck_ntt(&b);
#elif D == 0
#else
#    error "Not yet implemented."
#endif
    for (i = 0; i < K; ++i) {
        A1[i].vec[0] = b.vec[i];
    }

    poly_sub(&wprime, &z1tmp.vec[0], c);
    poly_lsb(&wprime, &wprime);

    polyvecl_ntt(&z1tmp);
    polymatkl_pointwise_montgomery(&highbits, A1, &z1tmp);
    polyveck_invntt_tomont(&highbits);
    polyveck_poly_fromcrt(&highbits, &highbits, &wprime);
    polyveck_freeze2q(&highbits);

    *out_raw = highbits;
    polyveck_highbits_hint(out_w0, &highbits);
    *out_w1 = *out_w0;
    polyveck_add(out_w1, out_w1, h);
    polyveck_csubDQ2ALPHA(out_w1);

    polyveck_mul_alpha(out_z2, out_w1);
    polyveck_sub(out_z2, out_z2, &highbits);
    poly_add(&out_z2->vec[0], &out_z2->vec[0], &wprime);
    polyveck_reduce2q(out_z2);
    polyveck_div2(out_z2);
}

static int sign_and_collect_hint(
    polyvecl *out_z1rnd, polyveck *out_z2rnd, polyvecl *out_pub_z1,
    polyveck *out_pub_z2, poly *out_c, polyveck *out_h, polyveck *out_w0,
    polyveck *out_w1, polyveck *out_raw, int32_t *out_b, const uint8_t *m,
    size_t mlen, const uint8_t *pk, const uint8_t *sk)
{
    uint8_t buf[POLYVECK_HIGHBITS_PACKEDBYTES + POLYC_PACKEDBYTES] = {0};
    uint8_t sig[HAETAE_CRYPTO_BYTES] = {0};
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
    polyvecl z1rnd, hb_z1, lb_z1;
    polyveck z2rnd, z2rnd_out, h, htmp;
    poly c, chat, z1rnd0, lsb;
    xof256_state state;
    unsigned int i;

    unpack_sk(A1, &s1, &s2, key, sk);

    xof256_absorbe_twice(&state, sk, HAETAE_CRYPTO_PUBLICKEYBYTES, m,
                         mlen);
    xof256_squeeze(mu, CRHBYTES, &state);
    shake256_inc_ctx_release(&state);
    xof256_absorbe_twice(&state, key, SEEDBYTES, mu, CRHBYTES);
    xof256_squeeze(seedbuf, CRHBYTES, &state);
    shake256_inc_ctx_release(&state);

    polyvecm_ntt(&s1);
    polyveck_ntt(&s2);

reject:
    counter =
        polyfixveclk_sample_hyperball(&y1, &y2, &b, seedbuf, counter);

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

    reject1 =
        ((uint64_t)B1SQ * LN * LN - polyfixveclk_sqnorm2(&z1, &z2)) >> 63;
    reject1 &= 1;

    polyfixvecl_double(&z1tmp, &z1);
    polyfixveck_double(&z2tmp, &z2);
    polyfixfixvecl_sub(&z1tmp, &z1tmp, &y1);
    polyfixfixveck_sub(&z2tmp, &z2tmp, &y2);

    reject2 = (polyfixveclk_sqnorm2(&z1tmp, &z2tmp) -
               (uint64_t)B0SQ * LN * LN) >>
              63;
    reject2 &= 1;
    reject2 &= (b & 0x2) >> 1;

    if (reject1 | reject2) {
        goto reject;
    }

    polyfixvecl_round(&z1rnd, &z1);
    polyfixveck_round(&z2rnd, &z2);
    z2rnd_out = z2rnd;

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

    *out_z1rnd = z1rnd;
    *out_z2rnd = z2rnd_out;
    *out_c = c;
    *out_h = h;
    *out_b = (int32_t)(b & 0x3);
    reconstruct_hint_response(out_pub_z1, out_pub_z2, out_w0, out_w1,
                              out_raw, pk, &c, &lb_z1, &hb_z1, &h);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_sigs> <output_file>\n", argv[0]);
        return 1;
    }

    uint32_t num_sigs = (uint32_t)atoi(argv[1]);
    const char *outfile = argv[2];
    FILE *fp = (strcmp(outfile, "-") == 0) ? stdout : fopen(outfile, "wb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s for writing\n", outfile);
        return 1;
    }

    fprintf(stderr, "HAETAE-%d hint-view signature collection\n",
            HAETAE_MODE);
    fprintf(stderr, "  N=%d, K=%d, L=%d, M=%d, LN=%d\n", N, K, L, M, LN);
    fprintf(stderr, "  Collecting %u signatures...\n", num_sigs);

    uint8_t pk[HAETAE_CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[HAETAE_CRYPTO_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);

    polyvecl A1_dummy[K];
    polyvecm s1;
    polyveck s2;
    uint8_t key_dummy[SEEDBYTES];
    unpack_sk(A1_dummy, &s1, &s2, key_dummy, sk);

    uint32_t rec_i32 = (uint32_t)((L + 6 * K + 1) * N + 1);
    uint32_t header[8] = {HINT_VIEW_MAGIC, N,      K, L, M, LN,
                          num_sigs,        rec_i32};
    fwrite(header, sizeof(uint32_t), 8, fp);

    for (unsigned int i = 0; i < M; i++) {
        fwrite(s1.vec[i].coeffs, sizeof(int32_t), N, fp);
    }
    for (unsigned int i = 0; i < K; i++) {
        fwrite(s2.vec[i].coeffs, sizeof(int32_t), N, fp);
    }

    polyvecl z1rnd, pub_z1;
    polyveck z2rnd, pub_z2, h, w0, w1, raw;
    poly c;
    int32_t bdiag;
    uint8_t msg[32];
    clock_t start = clock();

    for (uint32_t t = 0; t < num_sigs; t++) {
        memcpy(msg, &t, sizeof(t));
        memset(msg + sizeof(t), 0, sizeof(msg) - sizeof(t));

        sign_and_collect_hint(&z1rnd, &z2rnd, &pub_z1, &pub_z2, &c, &h,
                              &w0, &w1, &raw, &bdiag, msg, sizeof(msg), pk,
                              sk);

        for (unsigned int i = 0; i < L; i++) {
            fwrite(z1rnd.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        for (unsigned int i = 0; i < K; i++) {
            fwrite(z2rnd.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        for (unsigned int i = 0; i < K; i++) {
            fwrite(pub_z2.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        fwrite(c.coeffs, sizeof(int32_t), N, fp);
        for (unsigned int i = 0; i < K; i++) {
            fwrite(h.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        for (unsigned int i = 0; i < K; i++) {
            fwrite(w0.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        for (unsigned int i = 0; i < K; i++) {
            fwrite(w1.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        for (unsigned int i = 0; i < K; i++) {
            fwrite(raw.vec[i].coeffs, sizeof(int32_t), N, fp);
        }
        fwrite(&bdiag, sizeof(int32_t), 1, fp);

        if ((t + 1) % 10000 == 0) {
            double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
            double rate = (t + 1) / elapsed;
            double eta = (num_sigs - t - 1) / rate;
            fprintf(stderr, "  [%u/%u] %.0f sigs/sec, ETA: %.0fs\n", t + 1,
                    num_sigs, rate, eta);
        }
    }

    if (fp != stdout) {
        fclose(fp);
    } else {
        fflush(fp);
    }

    double total_time = (double)(clock() - start) / CLOCKS_PER_SEC;
    fprintf(stderr, "Done! %u signatures in %.1fs (%.0f sigs/sec)\n",
            num_sigs, total_time, num_sigs / total_time);
    fprintf(stderr, "Output: %s\n", outfile);
    return 0;
}
