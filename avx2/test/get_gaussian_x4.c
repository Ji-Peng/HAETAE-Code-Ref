// Widening assay harness: dumps discrete-Gaussian samples produced by the
// VECTOR hot path (sample_gauss_N_4x), so we can confirm the AVX2 vector path
// (not just the scalar refill) reaches beyond the stock 4.82-sigma cap.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "api.h"
#include "sampler.h"

#define NL 256           // samples per lane per call
#define ITERS 1000       // 1000 * 4 lanes * 256 = ~1.02M samples

int main(void)
{
    uint64_t *r0 = malloc(NL * sizeof(uint64_t));
    uint64_t *r1 = malloc(NL * sizeof(uint64_t));
    uint64_t *r2 = malloc(NL * sizeof(uint64_t));
    uint64_t *r3 = malloc(NL * sizeof(uint64_t));
    uint8_t s0[NL / 8], s1[NL / 8], s2[NL / 8], s3[NL / 8];
    fp96_76 sqsum = {0};
    uint8_t seed[CRHBYTES] = {0};
    FILE *f = fopen("gauss_x4.txt", "w");
    if (!f) { perror("open"); return 1; }

    for (int it = 0; it < ITERS; it++) {
        uint16_t n = (uint16_t)(it * 4);
        sqsum.limb48[0] = sqsum.limb48[1] = 0;
        sample_gauss_N_4x(r0, r1, r2, r3, s0, s1, s2, s3, &sqsum, seed,
                          n, n + 1, n + 2, n + 3, NL, NL, NL, NL);
        for (int i = 0; i < NL; i++) {
            fprintf(f, "%llu\n%llu\n%llu\n%llu\n",
                    (unsigned long long)r0[i], (unsigned long long)r1[i],
                    (unsigned long long)r2[i], (unsigned long long)r3[i]);
        }
    }
    fclose(f);
    free(r0); free(r1); free(r2); free(r3);
    return 0;
}
