// Scalar unit tests for the SIGMA14 sampler (gates ii, iii of the build sheet).
// Built with -DSIGMA14; #includes sampler.c to reach the static gauss144 / approx_exp14.
#define SIGMA14
#include "../sampler.c"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef unsigned long long ull;

// reference: count #{ CDT14[i] < rand144 } via plain if-based lexicographic compare
// (144 bits do NOT fit __int128; compare hi, then mid, then lo). Independent of gauss144's
// borrow-bit trick.
static uint64_t ref_gauss144(uint64_t rlo, uint64_t rmid, uint64_t rhi) {
    uint64_t cnt = 0;
    for (unsigned i = 0; i < CDTLEN14; i++) {
        uint64_t clo = CDT14[i][0], cmid = CDT14[i][1], chi = CDT14[i][2];
        int lt;
        if (chi != rhi)       lt = (chi < rhi);
        else if (cmid != rmid) lt = (cmid < rmid);
        else                   lt = (clo < rlo);
        cnt += (uint64_t)lt;
    }
    return cnt;
}

static uint64_t rng_state = 0x1234567890abcdefULL;
static uint64_t xr(void) { rng_state ^= rng_state << 13; rng_state ^= rng_state >> 7; rng_state ^= rng_state << 17; return rng_state; }
static uint64_t r48(void) { return xr() & ((1ULL << 48) - 1); }

int main(void) {
    int fails = 0;

    // ---- gate (ii): gauss144 vs reference, random + edge cases ----
    long NREP = 2000000;
    for (long t = 0; t < NREP; t++) {
        uint64_t rlo = r48(), rmid = r48(), rhi = r48();
        if (gauss144(rlo, rmid, rhi) != ref_gauss144(rlo, rmid, rhi)) {
            printf("GAUSS144 MISMATCH at rlo=%llu rmid=%llu rhi=%llu: got %llu ref %llu\n",
                   (ull)rlo, (ull)rmid, (ull)rhi, (ull)gauss144(rlo, rmid, rhi), (ull)ref_gauss144(rlo, rmid, rhi));
            fails++; if (fails > 5) break;
        }
    }
    // edge cases: exactly equal to each CDT14[i] on all limbs (strict-less => not counted at that index)
    for (unsigned i = 0; i < CDTLEN14; i++) {
        uint64_t clo = CDT14[i][0], cmid = CDT14[i][1], chi = CDT14[i][2];
        if (gauss144(clo, cmid, chi) != ref_gauss144(clo, cmid, chi)) { printf("EDGE-EQ mismatch i=%u\n", i); fails++; }
    }
    // 0 and 2^144-1
    if (gauss144(0, 0, 0) != 0) { printf("EDGE zero: got %llu want 0\n", (ull)gauss144(0,0,0)); fails++; }
    uint64_t m48 = (1ULL << 48) - 1;
    if (gauss144(m48, m48, m48) != ref_gauss144(m48, m48, m48)) { printf("EDGE max mismatch\n"); fails++; }
    printf("[ii] gauss144: %ld random + %d edge + 2 corner; fails so far=%d\n", NREP, CDTLEN14, fails);
    printf("     x range spot: gauss144(max)=%llu (expect 222)\n", (ull)gauss144(m48, m48, m48));

    // ---- gate (iii): approx_exp14 accuracy vs true exp(-u)*2^48 ----
    // (bit-parity with avx2 is gate (i)/(iv) later; here: matches the math to <=3 ulp)
    uint64_t worst = 0; double worst_u = 0;
    long NG = 2000001;
    double NTH_MAX = 0.8730;
    for (long i = 0; i < NG; i++) {
        double u = (double)i / (NG - 1) * NTH_MAX;
        uint64_t exp_in = (uint64_t)llround(u * ldexp(1.0, 48));
        uint64_t got = approx_exp14(exp_in);
        // true = round(2^48 * exp(-exp_in/2^48)) in long double
        long double tu = (long double)exp_in / ldexpl(1.0L, 48);
        long double tv = ldexpl(1.0L, 48) * expl(-tu);
        uint64_t tru = (uint64_t)llroundl(tv);
        uint64_t e = got > tru ? got - tru : tru - got;
        if (e > worst) { worst = e; worst_u = u; }
    }
    printf("[iii] approx_exp14: max_abs_err=%llu ulp over [0,%.4f] (want <=3), worst u=%.4f\n",
           (ull)worst, NTH_MAX, worst_u);
    printf("      approx_exp14(0)=%llu (want 2^48=%llu)\n", (ull)approx_exp14(0), (ull)(1ULL<<48));
    if (worst > 3) fails++;

    // ---- T4: dump full-path samples (clean scalar sample_gauss_N) for tail analysis ----
    {
        const int NS = 4000, NIT = 500;   // 2M samples
        uint64_t *rr = malloc(NS * sizeof(uint64_t));
        uint8_t *signs = malloc(NS / 8 + 1);
        FILE *f = fopen("/tmp/samples_s14_clean.txt", "w");
        uint8_t seed[CRHBYTES] = {0};
        for (int it = 0; it < NIT; it++) {
            fp96_76 sqsum = {{0, 0}};
            sample_gauss_N(rr, signs, &sqsum, seed, (uint16_t)it, NS);
            for (int i = 0; i < NS; i++) {
                int sb = (signs[i / 8] >> (i % 8)) & 1;
                fprintf(f, "%s%llu\n", sb ? "-" : "", (ull)rr[i]);
            }
        }
        fclose(f); free(rr); free(signs);
        printf("[T4] dumped %d samples -> /tmp/samples_s14_clean.txt\n", NS * NIT);
    }

    printf(fails ? "\n*** SCALAR TESTS FAILED (%d) ***\n" : "\n=== SCALAR TESTS PASS ===\n", fails);
    return fails ? 1 : 0;
}
