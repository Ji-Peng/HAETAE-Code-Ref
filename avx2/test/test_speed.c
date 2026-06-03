#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "polyfix.h"
#include "cpucycles.h"
#include "speed_print.h"

#define NTESTS 10000
#define MLEN 24

uint64_t t[NTESTS];

int main(void)
{
    unsigned int i;
    uint16_t nonce = 0;
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char m[MLEN] = {0};
    unsigned char m2[MLEN + CRYPTO_BYTES];
    unsigned char sm[MLEN + CRYPTO_BYTES];
    uint8_t seed[CRHBYTES] = {0};
    uint8_t b = 0;
    polyfixvecl y1;
    polyfixveck y2;

    size_t mlen;
    size_t smlen;

    int result;
    mlen = MLEN;

    printf("CRYPTO_ALGNAME: %s\n", CRYPTO_ALGNAME);
    printf("CRYPTO_PUBLICKEYBYTES: %d\n", CRYPTO_PUBLICKEYBYTES);
    printf("CRYPTO_SECRETKEYBYTES: %d\n", CRYPTO_SECRETKEYBYTES);
    printf("CRYPTO_BYTES: %d\n", CRYPTO_BYTES);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        crypto_sign_keypair(pk, sk);
    }
    print_results("Keypair:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        crypto_sign(sm, &smlen, sm, mlen, sk);
    }
    print_results("Sign:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        nonce = polyfixveclk_sample_hyperball(&y1, &y2, &b, seed, nonce);
    }
    print_results("Sample_hyperball:", t, NTESTS);

    for (i = 0; i < NTESTS; ++i) {
        t[i] = cpucycles();
        result = crypto_sign_open(m2, &mlen, sm, smlen, pk);
    }
    print_results("Verify:", t, NTESTS);
    (void)result;

    return 0;
}
