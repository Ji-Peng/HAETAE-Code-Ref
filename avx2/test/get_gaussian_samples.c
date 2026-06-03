#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "api.h"
#include "sampler.h"

#define NUM_SAMPLES 1000        // 每次采样1000个点
#define NUM_ITERATIONS 1000     // 循环1000次，总共采样100万个点

void dump_gaussian_samples() {
    uint64_t *r = malloc(NUM_SAMPLES * sizeof(uint64_t));
    uint8_t *signs = malloc((NUM_SAMPLES / 8 + 1) * sizeof(uint8_t));
    fp96_76 sqsum = {0}; 
    uint8_t seed[CRHBYTES] = {0}; // 测试用的全零种子
    uint16_t nonce = 0;
    uint64_t total_samples = 0;

    printf("Start sampling (%d iterations of %d samples)...\n", NUM_ITERATIONS, NUM_SAMPLES);
    
    // 第一次打开文件用写入模式，后续追加
    FILE *f = fopen("samples.txt", "w");
    if (!f) {
        perror("Failed to open file");
        free(r);
        free(signs);
        return;
    }

    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        // 调用代码中提供的 sample_gauss_N
        sample_gauss_N(r, signs, &sqsum, seed, nonce, NUM_SAMPLES);

        // 写入当前批次的样本
        for (size_t i = 0; i < NUM_SAMPLES; i++) {
            // 提取对应的符号位 (每字节包含8个样本的符号)
            int sign_bit = (signs[i / 8] >> (i % 8)) & 1;
            // 如果 sign_bit 为 1，视为负数；为 0 视为正数
            if (sign_bit) {
                fprintf(f, "-%llu\n", (unsigned long long)r[i]);
            } else {
                fprintf(f, "%llu\n", (unsigned long long)r[i]);
            }
        }
        
        total_samples += NUM_SAMPLES;
        
        // 定期输出进度
        if ((iter + 1) % 100 == 0) {
            printf("Progress: %llu samples written...\n", (unsigned long long)total_samples);
        }
    }
    
    fclose(f);
    free(r);
    free(signs);
    printf("Sampling completed. Total %llu samples written to samples.txt successfully.\n", 
           (unsigned long long)total_samples);
}

int main() {
    dump_gaussian_samples();
    return 0;
}