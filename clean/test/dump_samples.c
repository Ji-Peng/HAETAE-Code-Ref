// Mode-agnostic Gaussian sample dumper (builds with or without -DSIGMA14).
// Dumps signed magnitudes from the clean scalar sample_gauss_N to argv[1].
// Used for the base-vs-s14 tail-widening assay (gate v) on the clean path.
#include "sampler.h"
#include "fixpoint.h"
#include "symmetric.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv) {
    const char *out = argc > 1 ? argv[1] : "samples.txt";
    long total = argc > 2 ? atol(argv[2]) : 5000000;
    const int NS = 5000;
    uint64_t *r = malloc(NS * sizeof(uint64_t));
    uint8_t *signs = malloc(NS / 8 + 1);
    uint8_t seed[CRHBYTES] = {0};
    FILE *f = fopen(out, "w");
    long done = 0;
    for (uint16_t it = 0; done < total; it++) {
        fp96_76 sqsum = {{0, 0}};
        sample_gauss_N(r, signs, &sqsum, seed, it, NS);
        for (int i = 0; i < NS && done < total; i++, done++) {
            int sb = (signs[i / 8] >> (i % 8)) & 1;
            fprintf(f, "%s%llu\n", sb ? "-" : "", (unsigned long long)r[i]);
        }
    }
    fclose(f); free(r); free(signs);
    fprintf(stderr, "dumped %ld samples -> %s\n", done, out);
    return 0;
}
