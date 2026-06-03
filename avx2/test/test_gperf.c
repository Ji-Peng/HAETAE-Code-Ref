#include <gperftools/profiler.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api.h"

static void gperf_sign()
{
    uint8_t pk[HAETAE_CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[HAETAE_CRYPTO_SECRETKEYBYTES];
    uint8_t sig[24 + HAETAE_CRYPTO_BYTES];
    size_t siglen;
    size_t msglen = 24;

    // Keypair generation
    if (crypto_sign_keypair(pk, sk) != 0) {
        fprintf(stderr, "Keypair generation failed\n");
        exit(EXIT_FAILURE);
    }

    const char *basename = "gperf_sign";
    char profname[64];
    char textname[64];

    snprintf(profname, sizeof(profname), "%s.prof", basename);
    snprintf(textname, sizeof(textname), "%s.txt", basename);
    printf("gperf the signature generation function. Filename: %s\n",
           profname);
    ProfilerStart(profname);
    for (int i = 0; i < 10000; i++) {
        crypto_sign(sig, &siglen, sig, msglen, sk);
    }
    ProfilerStop();
    printf("Run the command: pprof --text ./out/test_gperf %s > %s\n",
           profname, textname);
}

int main()
{
    gperf_sign();
    return 0;
}