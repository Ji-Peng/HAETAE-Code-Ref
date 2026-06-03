# HAETAE 球采样 (Hyperball Sampling) Profiling Report

## 环境

- **CPU**: 通过 `rdtsc` 测量周期数
- **编译器**: GCC, clean 用 `-O3`, avx2 用 `-O3 -mavx2 -march=native`
- **Profiling 工具**: gperftools CPU Profiler (采样频率 10KHz)
- **参数**: HAETAE2 (N=256, K=2, L=4), HAETAE3 (N=256, K=3, L=6), HAETAE5 (N=256, K=4, L=7)

---

## 1. 组件级耗时 (CPU Cycles)

### HAETAE2 (K=2, L=4)

| 组件 | Clean median | Clean avg | AVX2 median | AVX2 avg | 加速比 |
|------|-------------|-----------|-------------|----------|--------|
| sample_gauss16 (CDT) | 26 | 26 | 26 | 26 | 1.0x |
| approx_exp | 26 | 31 | 26 | 31 | 1.0x |
| fixpoint_square | 26 | 23 | 26 | 23 | 1.0x |
| fixpoint_newton_invsqrt | 269 | 266 | 296 | 300 | 0.9x |
| fixpoint_mul_rnd13 | 26 | 24 | 26 | 24 | 1.0x |
| sample_gauss_N (N+1) | 47,573 | 47,643 | 48,194 | 48,227 | 1.0x |
| sample_gauss_N_4x (4×N) | — | — | 76,220 | 76,597 | — |
| polyfixveclk_sqnorm2 | 1,592 | 1,608 | 215 | 224 | **7.4x** |
| **sample_hyperball (总)** | **303,803** | **305,209** | **156,491** | **157,331** | **1.94x** |

### HAETAE3 (K=3, L=6)

| 组件 | Clean median | Clean avg | AVX2 median | AVX2 avg | 加速比 |
|------|-------------|-----------|-------------|----------|--------|
| sample_gauss_N (N+1) | 47,681 | 47,793 | 48,491 | 48,448 | 1.0x |
| sample_gauss_N_4x (4×N) | — | — | 76,166 | 76,560 | — |
| polyfixveclk_sqnorm2 | 2,429 | 2,448 | 377 | 390 | **6.4x** |
| **sample_hyperball (总)** | **457,271** | **459,674** | **205,361** | **206,609** | **2.23x** |

### HAETAE5 (K=4, L=7)

| 组件 | Clean median | Clean avg | AVX2 median | AVX2 avg | 加速比 |
|------|-------------|-----------|-------------|----------|--------|
| sample_gauss_N (N+1) | 47,627 | 47,698 | 47,735 | 47,741 | 1.0x |
| sample_gauss_N_4x (4×N) | — | — | 76,274 | 76,669 | — |
| polyfixveclk_sqnorm2 | 2,969 | 2,981 | 458 | 460 | **6.5x** |
| **sample_hyperball (总)** | **555,578** | **557,707** | **235,115** | **236,364** | **2.36x** |

### 关键观察

1. **单个组件（CDT、approx_exp、square、mul_rnd13）** 的标量耗时极低（~26 cycles），说明瓶颈不在单次调用而在调用次数。
2. **sample_gauss_N_4x 的吞吐量优势**：4 路并行处理 4×N 个样本仅需 ~76K cycles，而单路 1×(N+1) 需 ~48K cycles。等效吞吐量 ~2.5x。
3. **sqnorm2 在 AVX2 中获得 6-7x 加速**，但它只占总时间的 ~0.5%（clean）/ ~0.1%（avx2）。
4. **fixpoint_newton_invsqrt** 仅 ~270 cycles，占总时间 < 0.1%。

---

## 2. Profiling 结果 — Clean 实现 (HAETAE2)

1411 个有效采样，`sample_hyperball` 每次调用约 304K cycles。

### Flat Profile (Self Time)

| 函数 | Self % | Cum % | 说明 |
|------|--------|-------|------|
| KeccakF1600_StatePermute | 32.2% | 32.2% | Keccak-f[1600] 核心置换 |
| smulh48 | 18.6% | 18.6% | 48-bit 有符号高位乘法 (approx_exp 核心) |
| keccak_inc_squeeze | 9.0% | 41.0% | SHAKE-256 squeeze 调度 |
| sample_gauss16 | 6.8% | 6.8% | CDT 查表采样 |
| fixpoint_mul | 4.5% | 12.6% | 定点乘法 |
| mulacc48 | 4.5% | 5.7% | 48-bit 乘加 |
| sample_gauss_sigma76 | 4.5% | 37.9% | 单个高斯样本 |
| sample_gauss | 4.3% | 42.3% | 批量高斯采样循环 |
| mul48 | 3.5% | 4.7% | 48-bit 乘法 |
| approx_exp | 3.0% | 21.7% | 近似指数 (rejection test) |
| fixpoint_square | 2.6% | 4.9% | 定点平方 |
| fixpoint_mul_rnd13 | 1.6% | 14.2% | 坐标缩放 |

### Cumulative Profile (调用树视角)

| 函数 | Cum % | 说明 |
|------|-------|------|
| polyfixveclk_sample_hyperball | 100% | 总入口 |
| sample_gauss_N | 83.8% | 高斯采样 (含 SHAKE-256) |
| sample_gauss | 42.3% | 采样循环 |
| keccak_inc_squeeze | 41.0% | SHAKE-256 随机数 |
| sample_gauss_sigma76 | 37.9% | 单样本 (CDT + rejection) |
| KeccakF1600_StatePermute | 32.2% | Keccak 核心 |
| approx_exp | 21.7% | 近似指数 |
| smulh48 | 18.6% | 48-bit 高位乘法 |
| fixpoint_mul_rnd13 | 14.2% | 坐标缩放 |
| fixpoint_mul | 12.6% | 定点乘法 |

### 成本分层

```
polyfixveclk_sample_hyperball (~304K cycles, 100%)
├── sample_gauss_N × (K+L) batches (83.8%)
│   ├── SHAKE-256 随机数生成 (41.0%)
│   │   ├── KeccakF1600_StatePermute (32.2%)  ← 绝对热点
│   │   └── keccak_inc_squeeze 调度 (9.0%)
│   └── Gaussian 采样算术 (42.8%)
│       ├── approx_exp / smulh48 (21.7%)  ← 第二热点
│       ├── fixpoint_mul_rnd13 (14.2%)  ← 坐标缩放
│       ├── sample_gauss16 / CDT (6.8%)
│       └── fixpoint_square (4.9%)
├── polyfixveclk_sqnorm2 (0.4%)
└── fixpoint_newton_invsqrt (0.4%)
```

---

## 3. Profiling 结果 — AVX2 实现 (HAETAE2)

1555 个有效采样，`sample_hyperball` 每次调用约 156K cycles。

### Flat Profile (Self Time)

| 函数 | Self % | Cum % | 说明 |
|------|--------|-------|------|
| smulh48_avx | 49.4% | 68.2% | AVX2 48-bit 有符号高位乘法 |
| looptop (Keccak4x) | 16.1% | 16.1% | 4路并行 Keccak 置换核心 |
| _mm256_add_epi64 | 8.7% | 8.7% | AVX2 64-bit 加法 (inline) |
| _mm256_mul_epi32 | 4.5% | 4.5% | AVX2 32-bit 乘法 (inline) |
| approx_exp_4x | 4.3% | 75.6% | 4路并行近似指数 |
| _mm256_slli_epi64 | 4.2% | 4.2% | AVX2 左移 (inline) |
| _mm256_or_si256 | 3.4% | 3.4% | AVX2 位或 (inline) |
| sample_gauss16_x4 | 1.8% | 3.3% | 4路并行 CDT 采样 |

### Cumulative Profile (调用树视角)

| 函数 | Cum % | 说明 |
|------|-------|------|
| polyfixveclk_sample_hyperball | 100% | 总入口 |
| sample_gauss_N_4x | 98.8% | 4路并行高斯采样 |
| approx_exp_4x | 75.6% | **4路近似指数 ← 新的绝对热点** |
| smulh48_avx | 68.2% | AVX2 高位乘法 (approx_exp 内部) |
| looptop (Keccak4x) | 16.1% | 4路 Keccak 置换 |
| sample_gauss16_x4 | 3.3% | 4路 CDT |
| sample_candidates_x4 | 1.4% | 候选样本构造 |

### 成本分层

```
polyfixveclk_sample_hyperball (~156K cycles, 100%)
├── sample_gauss_N_4x × 2 轮 (98.8%)
│   ├── approx_exp_4x (75.6%)  ← 新绝对热点
│   │   └── smulh48_avx (68.2%)  ← 49.4% self time
│   ├── Keccak4x (looptop) (16.1%)
│   ├── sample_gauss16_x4 (3.3%)
│   ├── sample_candidates_x4 (1.4%)
│   ├── rej_msk_4x (0.8%)
│   ├── _mul_rnd13 (0.8%)
│   └── move_to_mem_4x (0.5%)
├── fixpoint_newton_invsqrt (0.3%)
└── sum_sqr_4x (0.3%)
```

---

## 4. Clean vs AVX2 对比分析

### 瓶颈转移

| 组件 | Clean 占比 | AVX2 占比 | 说明 |
|------|-----------|-----------|------|
| SHAKE-256 (Keccak) | 41.0% | 16.1% | Keccak4x 并行化大幅降低 |
| approx_exp / smulh48 | 21.7% | **75.6%** | 成为 AVX2 版本的绝对瓶颈 |
| CDT 采样 | 6.8% | 3.3% | AVX2 4路并行 CDT |
| 坐标缩放 | 14.2% | 0.8% | AVX2 _mul_rnd13 极快 |
| 范数检查 | 0.4% | < 0.1% | AVX2 向量化范数 |
| invsqrt | 0.4% | 0.3% | 两版本都可忽略 |

### 关键结论

1. **Clean 版本**：SHAKE-256 (41%) 和 approx_exp (22%) 是两大瓶颈，各占约一半；坐标缩放 fixpoint_mul_rnd13 也占 14.2%。

2. **AVX2 版本**：Keccak4x 将 SHAKE-256 成本压缩至 16%，但 **approx_exp_4x 中的 smulh48_avx 占了 49.4% 的 self time**，成为绝对瓶颈。这是因为 `smulh48_avx` 需要模拟 48-bit 有符号高位乘法（AVX2 没有原生 64×64→128 乘法指令），涉及大量移位、符号扩展和乘加操作。

3. **加速比**：AVX2 相比 Clean 的总体加速为 **1.94x (HAETAE2)、2.23x (HAETAE3)、2.36x (HAETAE5)**。参数越大加速比越高，因为 sample_gauss_N_4x 的 4 路并行效率随批次数增加而提升。

### 优化方向

| 方向 | 目标 | 预期收益 | 说明 |
|------|------|---------|------|
| 优化 smulh48_avx | approx_exp_4x | ~20-30% | 减少符号扩展指令，或改用不同的 approx_exp 算法 |
| 使用 AVX-512 IFMA | smulh48 | ~30-40% | `_mm256_madd52hi_epu64` 可直接做 52-bit 高位乘 |
| 简化 approx_exp 多项式 | approx_exp | ~10-15% | 降低多项式阶数（当前 7 级） |

---

## 复现步骤

### 1. 编译

```bash
# Clean
cd HAETAE_Code/clean
make speed_hyperball    # 编译 speed test
make gperf_hyperball    # 编译 profiling (需安装 libgoogle-perftools-dev)

# AVX2
cd HAETAE_Code/avx2
make speed_hyperball
make gperf_hyperball
```

### 2. 运行 Speed Test

```bash
# Clean
make run_speed_hyperball    # 结果保存到 speed_hyperball.txt

# AVX2
make run_speed_hyperball    # 结果保存到 speed_hyperball.txt
```

### 3. 运行 Profiling

```bash
# Clean (HAETAE2 为例)
CPUPROFILE=hyperball_clean.prof CPUPROFILE_FREQUENCY=10000 ./out/gperf_hyperball2
google-pprof --text ./out/gperf_hyperball2 hyperball_clean.prof
google-pprof --cum --text ./out/gperf_hyperball2 hyperball_clean.prof

# AVX2
CPUPROFILE=hyperball_avx2.prof CPUPROFILE_FREQUENCY=10000 ./out/gperf_hyperball2
google-pprof --text ./out/gperf_hyperball2 hyperball_avx2.prof
google-pprof --cum --text ./out/gperf_hyperball2 hyperball_avx2.prof
```
