/*
 * speed_hyperball.c — HAETAE clean: 球采样各组件耗时测试 (CPU cycles)
 *
 * 测试组件:
 *   1. sample_gauss16       — CDT 查表采样
 *   2. sample_gauss_sigma76 — 单个高斯样本 (CDT + rejection)
 *   3. sample_gauss_N       — 一批 N 个高斯样本 (含 SHAKE-256)
 *   4. fixpoint_square      — 定点数平方
 *   5. fixpoint_newton_invsqrt — Newton 法求逆平方根
 *   6. fixpoint_mul_rnd13   — 坐标缩放乘法
 *   7. polyfixveclk_sqnorm2 — 范数检查
 *   8. polyfixveclk_sample_hyperball — 完整球采样
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpucycles.h"
#include "fixpoint.h"
#include "params.h"
#include "polyfix.h"
#include "sampler.h"
#include "speed_print.h"

#define NTESTS 10000
#define NTESTS_HYPERBALL 1000

uint64_t t[NTESTS + 1];

/* ---- expose static functions for benchmarking ---- */

/* sample_gauss16: CDT lookup (same logic as in sampler.c) */
#define CDTLEN 64
static const uint32_t CDT_bench[CDTLEN] = {
    3266,  6520,  9748,  12938, 16079, 19159, 22168, 25096,
    27934, 30674, 33309, 35833, 38241, 40531, 42698, 44742,
    46663, 48460, 50135, 51690, 53128, 54454, 55670, 56781,
    57794, 58712, 59541, 60287, 60956, 61554, 62085, 62556,
    62972, 63337, 63657, 63936, 64178, 64388, 64569, 64724,
    64857, 64970, 65066, 65148, 65216, 65273, 65321, 65361,
    65394, 65422, 65444, 65463, 65478, 65490, 65500, 65508,
    65514, 65519, 65523, 65527, 65529, 65531, 65533, 65534
};

static uint64_t bench_sample_gauss16(const uint64_t rand16) {
    unsigned int i;
    uint64_t r = 0;
    for (i = 0; i < CDTLEN; i++) {
        r += (((uint64_t)CDT_bench[i] - rand16) >> 63) & 1;
    }
    return r;
}

/* approx_exp (same as sampler.c) */
static uint64_t bench_approx_exp(const uint64_t x) {
    int64_t result;
    result = -0x0000B6C6340925AELL;
    result = ((smulh48(result, x) + (1LL << 2)) >> 3) + 0x0000B4BD4DF85227LL;
    result = ((smulh48(result, x) + (1LL << 2)) >> 3) - 0x0000887F727491E2LL;
    result = ((smulh48(result, x) + (1LL << 1)) >> 2) + 0x0000AAAA643C7E8DLL;
    result = ((smulh48(result, x) + (1LL << 1)) >> 2) - 0x0000AAAAA98179E6LL;
    result = ((smulh48(result, x) + 1LL) >> 1) + 0x0000FFFFFFFB2E7ALL;
    result = ((smulh48(result, x) + 1LL) >> 1) - 0x0000FFFFFFFFF85FLL;
    result = ((smulh48(result, x))) + 0x0000FFFFFFFFFFFCLL;
    return result;
}

int main(void)
{
    unsigned int i;
    uint16_t nonce = 0;
    uint8_t seed[CRHBYTES] = {0};
    uint8_t b = 0;
    polyfixvecl y1;
    polyfixveck y2;
    uint64_t samples[N];
    uint8_t signs[N / 8];
    fp96_76 sqsum, invsqrt;
    volatile uint64_t dummy;

    printf("HAETAE%d clean — 球采样组件级 speed test\n", HAETAE_MODE);
    printf("N=%d, K=%d, L=%d\n\n", N, K, L);

    /* 1. sample_gauss16 (CDT lookup) */
    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        dummy = bench_sample_gauss16(i & 0xFFFF);
    }
    t[NTESTS] = cpucycles();
    print_results("sample_gauss16:", t, NTESTS);

    /* 2. approx_exp */
    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        dummy = bench_approx_exp(i * 100ULL);
    }
    t[NTESTS] = cpucycles();
    print_results("approx_exp:", t, NTESTS);

    /* 3. fixpoint_square */
    {
        fp96_76 x, sqx;
        x.limb48[0] = 0x123456789ABCULL;
        x.limb48[1] = 0x1234ULL;
        for (i = 0; i < NTESTS; ++i) {
            t[i] = cpucycles();
            fixpoint_square(&sqx, &x);
        }
        t[NTESTS] = cpucycles();
        print_results("fixpoint_square:", t, NTESTS);
    }

    /* 4. fixpoint_newton_invsqrt */
    {
        fp96_76 xhalf;
        /* typical value: sqsum/2 after sampling N*(K+L) gaussians */
        xhalf.limb48[0] = 0xABCDEF012345ULL;
        xhalf.limb48[1] = 0x789ABCULL;
        for (i = 0; i < NTESTS; ++i) {
            t[i] = cpucycles();
            fixpoint_newton_invsqrt(&invsqrt, &xhalf);
        }
        t[NTESTS] = cpucycles();
        print_results("fixpoint_newton_invsqrt:", t, NTESTS);
    }

    /* 5. fixpoint_mul_rnd13 */
    {
        fp96_76 scale;
        scale.limb48[0] = 0x123456789ABCULL;
        scale.limb48[1] = 0x5678ULL;
        for (i = 0; i < NTESTS; ++i) {
            t[i] = cpucycles();
            dummy = fixpoint_mul_rnd13(0x123456789ABCULL, &scale, i & 1);
        }
        t[NTESTS] = cpucycles();
        print_results("fixpoint_mul_rnd13:", t, NTESTS);
    }

    /* 6. sample_gauss_N (one batch of N+1 samples, with SHAKE-256) */
    for (i = 0; i < NTESTS; ++i) {
        sqsum.limb48[0] = 0;
        sqsum.limb48[1] = 0;
        t[i] = cpucycles();
        sample_gauss_N(samples, signs, &sqsum, seed, (uint16_t)i, N + 1);
    }
    t[NTESTS] = cpucycles();
    print_results("sample_gauss_N (N+1):", t, NTESTS);

    /* 7. polyfixveclk_sqnorm2 */
    memset(&y1, 0x12, sizeof(y1));
    memset(&y2, 0x34, sizeof(y2));
    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        dummy = polyfixveclk_sqnorm2(&y1, &y2);
    }
    t[NTESTS] = cpucycles();
    print_results("polyfixveclk_sqnorm2:", t, NTESTS);

    /* 8. polyfixveclk_sample_hyperball (full) */
    for (i = 0; i < NTESTS_HYPERBALL; ++i) {
        t[i] = cpucycles();
        nonce = polyfixveclk_sample_hyperball(&y1, &y2, &b, seed, nonce);
    }
    t[NTESTS_HYPERBALL] = cpucycles();
    print_results("polyfixveclk_sample_hyperball:", t, NTESTS_HYPERBALL);

    (void)dummy;
    return 0;
}
