# HAETAE 签名生成 RAM 占用分析

## 测量工具与方法

- **Valgrind Massif** (`--stacks=yes`)：精确测量堆 + 栈的峰值占用
- **Valgrind Memcheck** (`--leak-check=full`)：检测内存泄漏
- **编译选项**：`-O2 -g`（保留符号），AVX2 版本额外 `-mno-avx512f`（Valgrind 兼容）
- **测试**：keygen 1 次 + sign 100 次

---

## 1. 内存泄漏修复

### 修复前：Clean 版本存在严重内存泄漏

```
total heap usage: 9,880 allocs, 1,101 frees → 泄漏 8,779 个 malloc 块
definitely lost: 1,826,032 bytes (1.74 MB)
```

**泄漏根因**：`fips202.c` 中每次 `shake256_inc_init()` / `shake128_inc_init()` 调用 `malloc(208)`
分配 Keccak state，但使用后从未调用 `shake256_inc_ctx_release()` / `shake128_inc_ctx_release()` 释放。

| 泄漏源 | 每次签名泄漏 | 100 次签名累计 |
|--------|------------|---------------|
| `sample_gauss_N` → `stream256_init` (6 次/签名) | 6 x 208 = 1,248 B | 124,800 B |
| `poly_challenge` → `shake256_absorb_twice` | 1 x 208 = 208 B | 20,800 B |
| `sign_signature` → `xof256_absorbe_twice` (2 次) | 2 x 208 = 416 B | 41,600 B |
| `unpack_sk` → `poly_uniform` / `polymatkl_expand` | ~10 x 208 | ~208,000 B |
| **合计** | ~4 KB/签名 | ~1.5 MB / 100 签名 |

### 修复方法

在以下文件的每个 `stream*_init` / `xof256_absorbe_*` 使用点之后添加对应的 `_ctx_release` 调用：

| 文件 | 修复点 | 释放调用 |
|------|--------|---------|
| `sampler.c` (`sample_gauss_N`) | 函数末尾 | `shake256_inc_ctx_release(&state)` |
| `poly.c` (`poly_uniform`) | 函数末尾 | `shake128_inc_ctx_release(&state)` |
| `poly.c` (`poly_uniform_eta`) | 函数末尾 | `shake256_inc_ctx_release(&state)` |
| `poly.c` (`poly_challenge`) | 两个 `#if` 分支末尾 | `shake256_inc_ctx_release(&state)` |
| `sign.c` (`crypto_sign_keypair`) | `xof256_squeeze` 后 | `shake256_inc_ctx_release(&state)` |
| `sign.c` (`crypto_sign_signature`) | 两处 `xof256_squeeze` 后 | `shake256_inc_ctx_release(&state)` |
| `sign.c` (`crypto_sign_verify`) | `xof256_squeeze` 后 | `shake256_inc_ctx_release(&state)` |
| `collect_signatures.c` (2 个文件) | 两处 `xof256_squeeze` 后 | `shake256_inc_ctx_release(&state)` |

### 修复后验证

```
total heap usage: 10,755 allocs, 10,755 frees → 零泄漏
definitely lost: 0 bytes in 0 blocks
```

---

## 2. Massif 峰值内存（修复后）

| 参数集 | Clean 堆 | Clean 栈 | **Clean 总计** | AVX2 堆 | AVX2 栈 | **AVX2 总计** |
|--------|---------|---------|------------|---------|---------|------------|
| HAETAE2 | 4.2 KB | 85 KB | **89 KB** | 4 KB | 146 KB | **150 KB** |
| HAETAE3 | 4.2 KB | 130 KB | **134 KB** | 4 KB | 190 KB | **194 KB** |
| HAETAE5 | 4.2 KB | 164 KB | **168 KB** | 4 KB | 225 KB | **229 KB** |

对比修复前：

| 参数集 | Clean 修复前 | Clean 修复后 | 降幅 |
|--------|------------|------------|------|
| HAETAE2 | 1,616 KB | **89 KB** | **18x** |
| HAETAE3 | 1,602 KB | **134 KB** | **12x** |
| HAETAE5 | 973 KB | **168 KB** | **5.8x** |

Clean 堆占用从 ~1.5 MB 降至 **4.2 KB**（= 4,096 B stdio 缓冲 + 208 B 瞬时 Keccak state）。

**AVX2 版本无此问题**：`sample_gauss_N_4x` 使用栈上的 `keccakx4_state`，不涉及 `malloc`。唯一的堆分配是 `printf` 的 stdio 缓冲区（4 KB）。

---

## 3. 签名函数栈帧分析

### crypto_sign_signature 栈帧

| 数据结构 | 类型 | HAETAE2 | HAETAE3 | HAETAE5 |
|---------|------|---------|---------|---------|
| `s1` | polyvecm (M x poly) | 3,072 B | 5,120 B | 6,144 B |
| `A1[K]` | polyvecl[K] | 8,192 B | 18,432 B | 28,672 B |
| `cs1` | polyvecl | 4,096 B | 6,144 B | 7,168 B |
| `s2, cs2, highbits, Ay` | 4 x polyveck | 8,192 B | 12,288 B | 16,384 B |
| `y1, z1, z1tmp` | 3 x polyfixvecl | 12,288 B | 18,432 B | 21,504 B |
| `y2, z2, z2tmp` | 3 x polyfixveck | 6,144 B | 9,216 B | 12,288 B |
| `z1rnd, hb_z1, lb_z1` | 3 x polyvecl | 12,288 B | 18,432 B | 21,504 B |
| `z2rnd, h, htmp` | 3 x polyveck | 6,144 B | 9,216 B | 12,288 B |
| `c, chat, z1rnd0, lsb` | 4 x poly | 4,096 B | 4,096 B | 4,096 B |
| `buf, seedbuf, key, mu` | 各种 | 768 B | 1,120 B | 1,184 B |
| **小计** | | **63.8 KB** | **100.0 KB** | **128.3 KB** |

### polyfixveclk_sample_hyperball 栈帧（嵌套在 sign 中）

| 数据结构 | 类型 | HAETAE2 | HAETAE3 | HAETAE5 |
|---------|------|---------|---------|---------|
| `samples[N*(L+K)]` | uint64_t[] | 12,288 B | 18,432 B | 22,528 B |
| `signs[N*(L+K)/8]` | uint8_t[] | 192 B | 288 B | 352 B |
| `sqsum, invsqrt` | 2 x fp96_76 | 32 B | 32 B | 32 B |
| **小计** | | **12.2 KB** | **18.3 KB** | **22.4 KB** |

### 理论栈总计 vs 实测

| 参数集 | sign + hyperball (理论) | Clean 实测栈 | AVX2 实测栈 |
|--------|----------------------|-------------|------------|
| HAETAE2 | 76.0 KB | 85 KB | 146 KB |
| HAETAE3 | 118.3 KB | 130 KB | 190 KB |
| HAETAE5 | 150.7 KB | 164 KB | 225 KB |

- Clean 实测比理论略高 ~10 KB：编译器对齐、返回地址、寄存器溢出等开销
- AVX2 实测多出 ~60-70 KB：`sample_gauss_N_4x` 内部的大量对齐 AVX2 数组

### AVX2 sample_gauss_N_4x 额外栈消耗

| 数据结构 | 大小 |
|---------|------|
| `outbuf` (4 路 SHAKE-256 输出) | 19,040 B |
| `sqr` (2 x NUM_GAUSSIANS x 4 lanes) | 17,792 B |
| `sample_candidates` | 8,896 B |
| `exp` | 8,896 B |
| `rejection` | 8,896 B |
| `outbuf2[4]`, `buf[4]` 等 | 1,344 B |
| `keccakx4_state` | 800 B |
| **小计** | **64.1 KB** |

---

## 4. 数据结构大小速查表

基本类型：`poly = int32_t[256] = 1024 B`

| 类型 | HAETAE2 | HAETAE3 | HAETAE5 |
|------|---------|---------|---------|
| poly | 1,024 B | 1,024 B | 1,024 B |
| polyveck (K x poly) | 2,048 B | 3,072 B | 4,096 B |
| polyvecl (L x poly) | 4,096 B | 6,144 B | 7,168 B |
| polyvecm (M x poly) | 3,072 B | 5,120 B | 6,144 B |
| polyfixveck | 2,048 B | 3,072 B | 4,096 B |
| polyfixvecl | 4,096 B | 6,144 B | 7,168 B |
| fp96_76 | 16 B | 16 B | 16 B |
| shake256incctx | 8 B (ptr) | 8 B (ptr) | 8 B (ptr) |
| Keccak state (malloc'd) | 208 B | 208 B | 208 B |

---

## 复现步骤

```bash
# 安装 valgrind
sudo apt install valgrind

# 编译带调试符号的版本
cd HAETAE_Code/clean
gcc -O2 -g -DHAETAE_MODE=2 -I../avx2/common -I./ -Itest \
    test/mem_sign.c decompose.c encoding.c fft.c fixpoint.c ntt.c \
    packing.c poly.c polyfix.c polymat.c polyvec.c reduce.c sampler.c \
    sign.c symmetric-shake.c ../avx2/common/randombytes.c \
    ../avx2/common/fips202.c -o out/mem_sign_g2

# Massif 堆+栈 profiling
valgrind --tool=massif --stacks=yes --detailed-freq=1 \
    --massif-out-file=massif_clean2.out ./out/mem_sign_g2
ms_print massif_clean2.out

# 内存泄漏检测
valgrind --leak-check=full --show-leak-kinds=definite ./out/mem_sign_g2
```
