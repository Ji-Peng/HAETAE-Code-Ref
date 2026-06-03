/*
 * mem_sign.c — 只测 signature generation 的峰值内存
 * 用法: /usr/bin/time -v ./out/mem_sign2
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "api.h"

#define MLEN 24
#define NSIGN 100

int main(void)
{
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char sm[MLEN + CRYPTO_BYTES];
    size_t smlen;

    printf("%s: Sign-only memory test (%d iterations)\n", CRYPTO_ALGNAME, NSIGN);

    /* keygen once (unavoidable, need valid sk) */
    crypto_sign_keypair(pk, sk);

    /* only sign */
    for (int i = 0; i < NSIGN; i++) {
        memset(sm + CRYPTO_BYTES, 0x42, MLEN);
        crypto_sign(sm, &smlen, sm + CRYPTO_BYTES, MLEN, sk);
    }

    printf("Done. siglen=%zu\n", smlen);
    return 0;
}
