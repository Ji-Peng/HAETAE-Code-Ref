/*
 * gperf_hyperball.c — HAETAE clean: 球采样 gperftools CPU profiling
 *
 * 使用方法:
 *   make gperf_hyperball2   (或 gperf_hyperball3 / gperf_hyperball5)
 *   CPUPROFILE=hyperball.prof CPUPROFILE_FREQUENCY=10000 ./out/gperf_hyperball2
 *   google-pprof --text ./out/gperf_hyperball2 hyperball.prof
 *   google-pprof --cum --text ./out/gperf_hyperball2 hyperball.prof
 */
#include <gperftools/profiler.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "params.h"
#include "polyfix.h"

#ifndef N_ITERS
#define N_ITERS 10000
#endif

int main(void)
{
    uint8_t seed[CRHBYTES] = {0};
    uint8_t b = 0;
    uint16_t nonce = 0;
    polyfixvecl y1;
    polyfixveck y2;

    const char *profname = "hyperball_clean.prof";

    printf("HAETAE%d clean: gperftools profiling %d iterations of sample_hyperball\n",
           HAETAE_MODE, N_ITERS);

    ProfilerStart(profname);
    for (int i = 0; i < N_ITERS; i++) {
        nonce = polyfixveclk_sample_hyperball(&y1, &y2, &b, seed, nonce);
    }
    ProfilerStop();

    printf("Done. Profile saved to %s\n", profname);
    printf("Analyze with:\n");
    printf("  google-pprof --text ./out/gperf_hyperball%d %s\n", HAETAE_MODE, profname);
    printf("  google-pprof --cum --text ./out/gperf_hyperball%d %s\n", HAETAE_MODE, profname);

    return 0;
}
