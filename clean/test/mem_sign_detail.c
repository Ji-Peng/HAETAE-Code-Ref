/*
 * mem_sign_detail.c — Sign-only 峰值内存测量 (读 /proc/self/status)
 *
 * 报告: VmPeak, VmRSS, VmStk (栈), VmData (堆+数据段)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.h"

#define MLEN 24
#define NSIGN 100

static void print_mem(const char *label)
{
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;
    char line[256];
    printf("[%s]\n", label);
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmPeak", 6) == 0 ||
            strncmp(line, "VmRSS",  5) == 0 ||
            strncmp(line, "VmStk",  5) == 0 ||
            strncmp(line, "VmData", 6) == 0 ||
            strncmp(line, "VmSize", 6) == 0)
        {
            printf("  %s", line);
        }
    }
    fclose(f);
}

int main(void)
{
    printf("%s: Sign-only memory detail (%d iterations)\n\n", CRYPTO_ALGNAME, NSIGN);

    print_mem("Before keygen");

    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);

    print_mem("After keygen");

    unsigned char sm[MLEN + CRYPTO_BYTES];
    size_t smlen;

    /* Sign once to trigger peak stack */
    memset(sm + CRYPTO_BYTES, 0x42, MLEN);
    crypto_sign(sm, &smlen, sm + CRYPTO_BYTES, MLEN, sk);

    print_mem("After 1st sign");

    for (int i = 1; i < NSIGN; i++) {
        memset(sm + CRYPTO_BYTES, 0x42, MLEN);
        crypto_sign(sm, &smlen, sm + CRYPTO_BYTES, MLEN, sk);
    }

    print_mem("After 100 signs");

    printf("\nDone. siglen=%zu\n", smlen);
    return 0;
}
