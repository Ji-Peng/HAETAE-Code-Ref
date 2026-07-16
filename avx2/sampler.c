#include "sampler.h"

#include <stdint.h>

#include "fixpoint.h"
#include "symmetric.h"
#ifdef SIGMA14
#    include "sigma14_common.h"  // CDT14, EXP14, CDTLEN14, SIGMA14 geometry (single source)
#endif

/*************************************************
 * Name:        rej_uniform
 *
 * Description: Sample uniformly random coefficients in [0, Q-1] by
 *              performing rejection sampling on array of random bytes.
 *
 * Arguments:   - int32_t *a: pointer to output array (allocated)
 *              - unsigned int len: number of coefficients to be sampled
 *              - const uint8_t *buf: array of random bytes
 *              - unsigned int buflen: length of array of random bytes
 *
 * Returns number of sampled coefficients. Can be smaller than len if not
 * enough random bytes were given.
 **************************************************/
unsigned int rej_uniform(int32_t *a, unsigned int len, const uint8_t *buf,
                         unsigned int buflen)
{
    unsigned int ctr, pos;
    uint32_t t;

    ctr = pos = 0;
    while (ctr < len && pos + 2 <= buflen) {
        t = buf[pos++];
        t |= (uint32_t)buf[pos++] << 8;

        if (t < Q)
            a[ctr++] = t;
    }
    return ctr;
}

/*************************************************
 * Name:        rej_eta
 *
 * Description: Sample uniformly random coefficients in [-ETA, ETA] by
 *              performing rejection sampling on array of random bytes.
 *
 * Arguments:   - int32_t *a: pointer to output array (allocated)
 *              - unsigned int len: number of coefficients to be sampled
 *              - const uint8_t *buf: array of random bytes
 *              - unsigned int buflen: length of array of random bytes
 *
 * Returns number of sampled coefficients. Can be smaller than len if not
 * enough random bytes were given.
 **************************************************/
static int32_t mod3(uint8_t t)
{
    int32_t r;
    r = (t >> 4) + (t & 0xf);
    r = (r >> 2) + (r & 3);
    r = (r >> 2) + (r & 3);
    r = (r >> 2) + (r & 3);
    return r - (3 * (r >> 1));
}
static int32_t mod3_leq26(uint8_t t)
{
    int32_t r;
    r = (t >> 4) + (t & 0xf);
    r = (r >> 2) + (r & 3);
    r = (r >> 2) + (r & 3);
    return r - (3 * (r >> 1));
}
static int32_t mod3_leq8(uint8_t t)
{
    int32_t r;
    r = (t >> 2) + (t & 3);
    r = (r >> 2) + (r & 3);
    return r - (3 * (r >> 1));
}
unsigned int rej_eta(int32_t *a, unsigned int len, const uint8_t *buf,
                     unsigned int buflen)
{
    unsigned int ctr, pos;

    ctr = pos = 0;
    while (ctr < len && pos < buflen) {
#if ETA == 1
        uint32_t t = buf[pos++];
        if (t < 243) {
            // reduce mod 3
            a[ctr++] = mod3(t);

            if (ctr >= len)
                break;

            t *= 171;  // 171*3 = 1 mod 256
            t >>= 9;
            a[ctr++] = mod3(t);

            if (ctr >= len)
                break;

            t *= 171;
            t >>= 9;
            a[ctr++] = mod3_leq26(t);

            if (ctr >= len)
                break;

            t *= 171;
            t >>= 9;
            a[ctr++] = mod3_leq8(t);

            if (ctr >= len)
                break;

            t *= 171;
            t >>= 9;
            a[ctr++] = (int32_t)t - (int32_t)3 * (t >> 1);
        }
#elif ETA == 2
        uint32_t t0, t1;
        t0 = buf[pos] & 0x0F;
        t1 = buf[pos++] >> 4;
        if (t0 < 15) {
            t0 = t0 - (205 * t0 >> 10) * 5;
            a[ctr++] = 2 - t0;
        }
        if (t1 < 15 && ctr < len) {
            t1 = t1 - (205 * t1 >> 10) * 5;
            a[ctr++] = 2 - t1;
        }
#endif
    }
    return ctr;
}

#ifndef SIGMA14
static uint64_t approx_exp(const uint64_t x)
{
    int64_t result;
    result = -0x0000B6C6340925AELL;
    result =
        ((smulh48(result, x) + (1LL << 2)) >> 3) + 0x0000B4BD4DF85227LL;
    result =
        ((smulh48(result, x) + (1LL << 2)) >> 3) - 0x0000887F727491E2LL;
    result =
        ((smulh48(result, x) + (1LL << 1)) >> 2) + 0x0000AAAA643C7E8DLL;
    result =
        ((smulh48(result, x) + (1LL << 1)) >> 2) - 0x0000AAAAA98179E6LL;
    result = ((smulh48(result, x) + 1LL) >> 1) + 0x0000FFFFFFFB2E7ALL;
    result = ((smulh48(result, x) + 1LL) >> 1) - 0x0000FFFFFFFFF85FLL;
    result = ((smulh48(result, x))) + 0x0000FFFFFFFFFFFCLL;
    return result;
}
#else
// SIGMA14: exp(-u)*2^48 on u in [0, ~0.875] (N_th_max=0.869 at x=222).
// Degree-15 uniform smulh48 Horner; coeffs EXP14[k]=round((-1)^k 2^48/k!)
// from sigma14_common.h.  Byte-identical to clean/sampler.c approx_exp14.
static uint64_t approx_exp14(const uint64_t exp_in)
{
    int64_t result = EXP14[EXP14_DEG];
    for (int k = EXP14_DEG - 1; k >= 0; k--) {
        result = smulh48(result, exp_in) + EXP14[k];
    }
    return (uint64_t)result;
}
#endif

#ifndef SIGMA14
#    define CDTLEN 64
static const uint32_t CDT[CDTLEN] = {
    3266,  6520,  9748,  12938, 16079, 19159, 22168, 25096, 27934, 30674,
    33309, 35833, 38241, 40531, 42698, 44742, 46663, 48460, 50135, 51690,
    53128, 54454, 55670, 56781, 57794, 58712, 59541, 60287, 60956, 61554,
    62085, 62556, 62972, 63337, 63657, 63936, 64178, 64388, 64569, 64724,
    64857, 64970, 65066, 65148, 65216, 65273, 65321, 65361, 65394, 65422,
    65444, 65463, 65478, 65490, 65500, 65508, 65514, 65519, 65523, 65527,
    65529, 65531, 65533, 65534};

static uint64_t sample_gauss16(const uint64_t rand16)
{
    unsigned int i;
    uint64_t r = 0;
    for (i = 0; i < CDTLEN; i++) {
        r += (((uint64_t)CDT[i] - rand16) >> 63) & 1;
    }
    return r;
}
#else
// SIGMA14: 144-bit CDT (223 entries) -> base integer x in [0,222] (~13.94
// sigma).  x = #{ i : CDT14[i] < rand144 }, 3-limb lexicographic
// STRICT-less compare (hi>mid>lo), matching the stock strict-less tie
// convention.  Each limb < 2^48 so (c-r)>>63 is the borrow. Byte-identical
// to clean/sampler.c.
static uint64_t gauss144(uint64_t rlo, uint64_t rmid, uint64_t rhi)
{
    unsigned int i;
    uint64_t r = 0;
    for (i = 0; i < CDTLEN14; i++) {
        uint64_t clo = CDT14[i][0], cmid = CDT14[i][1], chi = CDT14[i][2];
        uint64_t lt_hi = ((chi - rhi) >> 63) & 1;  // chi  < rhi
        uint64_t eq_hi = (chi == rhi);
        uint64_t lt_md = ((cmid - rmid) >> 63) & 1;  // cmid < rmid
        uint64_t eq_md = (cmid == rmid);
        uint64_t lt_lo = ((clo - rlo) >> 63) & 1;  // clo  < rlo
        r += lt_hi | (eq_hi & (lt_md | (eq_md & lt_lo)));
    }
    return r;  // x in [0, 222]
}
#endif

#ifndef SIGMA14
#    define GAUSS_RAND (72 + 16 + 48)
#else
#    define GAUSS_RAND GAUSS_RAND_SIGMA14  // 72 + 144 + 48 = 264
#endif
#define GAUSS_RAND_BYTES ((GAUSS_RAND + 7) / 8)
static int sample_gauss_sigma76(uint64_t *r, fp96_76 *sqr,
                                const uint8_t rand[GAUSS_RAND_BYTES])
{
    uint64_t x, exp_in;
    fp96_76 y;

#ifndef SIGMA14
    const uint64_t rand_gauss16 = rand[0] | (((uint64_t)rand[1]) << 8);
    const uint64_t rand_rej =
        rand[2] | (((uint64_t)rand[3]) << 8) |
        (((uint64_t)rand[4]) << 16) | (((uint64_t)rand[5]) << 24) |
        (((uint64_t)rand[6]) << 32) | (((uint64_t)rand[7]) << 40);

    /*
     * 1) sample x from small discrete gaussian via CDT (16-bit randomness)
     *    x is in range [0..~64].
     */
    x = sample_gauss16(rand_gauss16);

#    ifdef TRUNC_CAP_X
    /* FACTORS EXPERIMENT ONLY: exaggerated tail truncation.  x in [0..64]
     * maps to ~x/16 sigma; reject candidates with x > TRUNC_CAP_X to cap
     * the tail (e.g. TRUNC_CAP_X=32 -> ~2 sigma).  Guarded so the baseline
     * build is unchanged. */
    if (x > (uint64_t)TRUNC_CAP_X)
        return 0;
#    endif

    /*
     * 2) Build a 79-bit candidate y = yrand + (x << 72).
     *  - limb48[0] stores the low 48 bits of "yrand" (rand[8..13]).
     *  - limb48[1] stores 24 bits of upper randomness (rand[14..16]) and
     * x.
     */
    y.limb48[0] = rand[8] | ((uint64_t)rand[9] << 8) |
                  ((uint64_t)rand[10] << 16) | ((uint64_t)rand[11] << 24) |
                  ((uint64_t)rand[12] << 32) | ((uint64_t)rand[13] << 40);
    y.limb48[1] = rand[14] | ((uint64_t)rand[15] << 8) |
                  ((uint64_t)rand[16] << 16) | (x << 24);
#else
    // SIGMA14 layout: CDT144(rand[0..17]) | rej48(rand[18..23]) |
    // ylow72(rand[24..32]).
    const uint64_t rlo =
        rand[0] | ((uint64_t)rand[1] << 8) | ((uint64_t)rand[2] << 16) |
        ((uint64_t)rand[3] << 24) | ((uint64_t)rand[4] << 32) |
        ((uint64_t)rand[5] << 40);
    const uint64_t rmid =
        rand[6] | ((uint64_t)rand[7] << 8) | ((uint64_t)rand[8] << 16) |
        ((uint64_t)rand[9] << 24) | ((uint64_t)rand[10] << 32) |
        ((uint64_t)rand[11] << 40);
    const uint64_t rhi =
        rand[12] | ((uint64_t)rand[13] << 8) | ((uint64_t)rand[14] << 16) |
        ((uint64_t)rand[15] << 24) | ((uint64_t)rand[16] << 32) |
        ((uint64_t)rand[17] << 40);
    const uint64_t rand_rej =
        rand[18] | ((uint64_t)rand[19] << 8) | ((uint64_t)rand[20] << 16) |
        ((uint64_t)rand[21] << 24) | ((uint64_t)rand[22] << 32) |
        ((uint64_t)rand[23] << 40);

    // sample x from the 144-bit CDT (x in [0,222], ~13.94 sigma coverage)
    x = gauss144(rlo, rmid, rhi);

    // y := append x to y (x<<24 leaves bits 32..47 of limb48[1] for square
    // carries)
    y.limb48[0] = rand[24] | ((uint64_t)rand[25] << 8) |
                  ((uint64_t)rand[26] << 16) | ((uint64_t)rand[27] << 24) |
                  ((uint64_t)rand[28] << 32) | ((uint64_t)rand[29] << 40);
    y.limb48[1] = rand[30] | ((uint64_t)rand[31] << 8) |
                  ((uint64_t)rand[32] << 16) | (x << 24);
#endif

    // r := round y
    *r = (y.limb48[0] >> 15) ^ (y.limb48[1] << 33);
    *r += 1; /* rounding */
    *r >>= 1;

    // sqr := y*y  (sqr[1] = y^2 >> 124, sqr[0] = (y^2 >> 76) & (2^48-1))
    fixpoint_square(sqr, &y);

    // exp_in := (sqr - ((x*x) << 68)) scaled by 2^48 (see clean/sampler.c)
    exp_in = sqr->limb48[1] - ((x * x) << (68 - 48));
    exp_in <<= 20;
    exp_in |= sqr->limb48[0] >> 28;
    exp_in += 1; /* rounding */
    exp_in >>= 1;

    // accept with prob approx_exp(exp_in); clear return value w.p. 1/2 if
    // r==0.
    return ((((int64_t)(rand_rej ^
                        (rand_rej & 1))  // clear LSB (zero-sample coin)
              - (int64_t)
#ifdef SIGMA14
                    approx_exp14(exp_in)
#else
                    approx_exp(exp_in)
#endif
                  ) >>
             63) &
            (((*r | -*r) >> 63) | rand_rej)) &
           1;
}

int sample_gauss(uint64_t *r, fp96_76 *sqsum, const uint8_t *buf,
                 const size_t buflen, const size_t len,
                 const int dont_write_last)
{
    const uint8_t *pos = buf;
    fp96_76 sqr;
    size_t bytecnt = buflen, coefcnt = 0, cnt = 0;
    int accepted;
    uint64_t dummy;

    while (coefcnt < len) {
        if (bytecnt < GAUSS_RAND_BYTES) {
            renormalize(sqsum);
            return coefcnt;
        }

        if (dont_write_last && coefcnt == len - 1) {
            accepted = sample_gauss_sigma76(&dummy, &sqr, pos);
        } else {
            accepted = sample_gauss_sigma76(&r[coefcnt], &sqr, pos);
        }
        cnt += 1;
        coefcnt += accepted;
        pos += GAUSS_RAND_BYTES;
        bytecnt -= GAUSS_RAND_BYTES;

        sqsum->limb48[0] += sqr.limb48[0] & -(int64_t)accepted;
        sqsum->limb48[1] += sqr.limb48[1] & -(int64_t)accepted;
    }

    renormalize(sqsum);
    return len;
}

#define POLY_HYPERBALL_BUFLEN (GAUSS_RAND_BYTES * N)
#define POLY_HYPERBALL_NBLOCKS                            \
    ((POLY_HYPERBALL_BUFLEN + STREAM256_BLOCKBYTES - 1) / \
     STREAM256_BLOCKBYTES)
void sample_gauss_N(uint64_t *r, uint8_t *signs, fp96_76 *sqsum,
                    const uint8_t seed[CRHBYTES], const uint16_t nonce,
                    const size_t len)
{
    uint8_t buf[POLY_HYPERBALL_NBLOCKS * STREAM256_BLOCKBYTES];
    size_t bytecnt, coefcnt, firstflag = 1;
    stream256_state state;
    stream256_init(&state, seed, nonce);

    stream256_squeezeblocks(buf, POLY_HYPERBALL_NBLOCKS, &state);
    for (size_t i = 0; i < len / 8; i++) {
        signs[i] = buf[i];
    }
    bytecnt = POLY_HYPERBALL_NBLOCKS * STREAM256_BLOCKBYTES - len / 8;
    coefcnt = sample_gauss(r, sqsum, buf + len / 8, bytecnt, len, len % N);
    while (coefcnt < len) {
        size_t off = bytecnt % GAUSS_RAND_BYTES;
        for (size_t i = 0; i < off; i++) {
            buf[i] = buf[bytecnt + len / 8 * firstflag - off + i];
        }
        stream256_squeezeblocks(buf + off, 1, &state);
        bytecnt = STREAM256_BLOCKBYTES + off;

        coefcnt += sample_gauss(r + coefcnt, sqsum, buf, bytecnt,
                                len - coefcnt, len % N);
        firstflag = 0;
    }
}
