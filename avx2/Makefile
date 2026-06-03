CC = gcc
LD = gcc
LDFLAGS = 
LIBS_GPERF = -Wl,-rpath -Wl,/usr/local/lib -lprofiler
# -MT $@: Set the target name in the dependency file to the current rule's target ($@).
# -MMD: Generate a dependency file, excluding system headers.
# -MP: Add empty phony targets for each dependency to prevent errors if headers are missing.
# -MF out/$*.d: Output the dependency file to the specified path (out/$*.d).
DEPFLAGS = -MT $@ -MMD -MP -MF out/$*.d

CFLAGS_COMM = -W -Wextra -Wundef -Wshadow -mavx2 -mbmi2 -mpopcnt -maes -march=native -mtune=native -Icommon -I./
CFLAGS = $(CFLAGS_COMM) -fomit-frame-pointer -O3
#  We use -fno-omit-frame-pointer and -g for profiling to get accurate call stacks, and -fno-inline and -fno-lto to prevent the compiler from optimizing away functions that we want to profile.
CFLAGS_GPERF = $(CFLAGS_COMM) -fno-omit-frame-pointer -fno-inline -fno-lto -g -O2

# COMMON_DIR= common
# COMMON_FILES= $(COMMON_DIR)/randombytes.c  $(COMMON_DIR)/cpucycles.c $(COMMON_DIR)/aes256ctr.c 
# COMMON_HEADERS= $(COMMON_DIR)/randombytes.h  $(COMMON_DIR)/cpucycles.h $(COMMON_DIR)/aes256ctr.h 

SRCS_KECCAK = common/keccak4x/KeccakP-1600-times4-SIMD256.c common/keccak4x/fips202x4.c common/Keccak_avx2/fips202.c
SRCS        = common/randombytes.c common/cpucycles.c common/aes256ctr.c consts.c decompose.c encoding.c fft.c fixpoint.c  packing.c  poly.c polyfix.c polymat.c polyvec.c reduce.c sampler.c samplerx4.c sign.c symmetric-shake.c keccakx4_vec.c $(SRCS_KECCAK)
SRCS_SPEED  = test/test_speed.c test/speed_print.c test/cpucycles.c
SRCS_SPEED_HB = test/speed_hyperball.c test/speed_print.c test/cpucycles.c
SRCS_GPERF  = test/test_gperf.c
SRCS_GPERF_HB = test/gperf_hyperball.c
SRCS_GAUSSIAN = test/get_gaussian_samples.c

ASMS		= f1600x4.S invntt.S ntt.S pointwise.S shuffle.S common/Keccak_avx2/KeccakP-1600-AVX2.S

OBJS_SPEED_2	= $(addprefix out/, $(SRCS:.c=_sp_2.o)) \
	$(addprefix out/, $(ASMS:.S=_sp_2.o)) $(addprefix out/, $(SRCS_SPEED:.c=_sp_2.o))
OBJS_GPERF_2	= $(addprefix out/, $(SRCS:.c=_gf_2.o)) \
	$(addprefix out/, $(ASMS:.S=_gf_2.o)) $(addprefix out/, $(SRCS_GPERF:.c=_gf_2.o))

OBJS_SPEED_3	= $(addprefix out/, $(SRCS:.c=_sp_3.o)) \
	$(addprefix out/, $(ASMS:.S=_sp_3.o)) $(addprefix out/, $(SRCS_SPEED:.c=_sp_3.o))
OBJS_GPERF_3	= $(addprefix out/, $(SRCS:.c=_gf_3.o)) \
	$(addprefix out/, $(ASMS:.S=_gf_3.o)) $(addprefix out/, $(SRCS_GPERF:.c=_gf_3.o))

OBJS_SPEED_5	= $(addprefix out/, $(SRCS:.c=_sp_5.o)) \
	$(addprefix out/, $(ASMS:.S=_sp_5.o)) $(addprefix out/, $(SRCS_SPEED:.c=_sp_5.o))
OBJS_GPERF_5	= $(addprefix out/, $(SRCS:.c=_gf_5.o)) \
	$(addprefix out/, $(ASMS:.S=_gf_5.o)) $(addprefix out/, $(SRCS_GPERF:.c=_gf_5.o))

OBJS_SPEED_HB_2	= $(addprefix out/, $(SRCS:.c=_sp_2.o)) \
	$(addprefix out/, $(ASMS:.S=_sp_2.o)) $(addprefix out/, $(SRCS_SPEED_HB:.c=_sp_2.o))
OBJS_SPEED_HB_3	= $(addprefix out/, $(SRCS:.c=_sp_3.o)) \
	$(addprefix out/, $(ASMS:.S=_sp_3.o)) $(addprefix out/, $(SRCS_SPEED_HB:.c=_sp_3.o))
OBJS_SPEED_HB_5	= $(addprefix out/, $(SRCS:.c=_sp_5.o)) \
	$(addprefix out/, $(ASMS:.S=_sp_5.o)) $(addprefix out/, $(SRCS_SPEED_HB:.c=_sp_5.o))

OBJS_GPERF_HB_2	= $(addprefix out/, $(SRCS:.c=_gf_2.o)) \
	$(addprefix out/, $(ASMS:.S=_gf_2.o)) $(addprefix out/, $(SRCS_GPERF_HB:.c=_gf_2.o))
OBJS_GPERF_HB_3	= $(addprefix out/, $(SRCS:.c=_gf_3.o)) \
	$(addprefix out/, $(ASMS:.S=_gf_3.o)) $(addprefix out/, $(SRCS_GPERF_HB:.c=_gf_3.o))
OBJS_GPERF_HB_5	= $(addprefix out/, $(SRCS:.c=_gf_5.o)) \
	$(addprefix out/, $(ASMS:.S=_gf_5.o)) $(addprefix out/, $(SRCS_GPERF_HB:.c=_gf_5.o))

OBJS_GAUSSIAN   = $(addprefix out/, $(SRCS:.c=_gaussian.o)) \
	$(addprefix out/, $(ASMS:.S=_gaussian.o)) $(addprefix out/, $(SRCS_GAUSSIAN:.c=_gaussian.o))
				  
.PHONY: all profiling clean speed_hyperball gperf_hyperball

all: out/test_speed2 out/test_speed3 out/test_speed5

speed_hyperball: out/speed_hyperball2 out/speed_hyperball3 out/speed_hyperball5

gperf_hyperball: out/gperf_hyperball2 out/gperf_hyperball3 out/gperf_hyperball5

gaussian: out/get_gaussian_samples

profiling: out/test_gperf5

run_speed: all
	./out/test_speed2 > speed.txt
	./out/test_speed3 >> speed.txt
	./out/test_speed5 >> speed.txt

run_speed_hyperball: speed_hyperball
	./out/speed_hyperball2 > speed_hyperball.txt
	./out/speed_hyperball3 >> speed_hyperball.txt
	./out/speed_hyperball5 >> speed_hyperball.txt

run_profiling: profiling
	./out/test_gperf5
	go tool pprof --text gperf_sign.prof > gperf_sign5.txt

run_gaussian: gaussian
	./out/get_gaussian_samples

clean:
	-rm -rf out/

out:
	@mkdir -p out
	@mkdir -p out/test
	@mkdir -p out/common
	@mkdir -p out/common/keccak4x
	@mkdir -p out/common/Keccak_avx2

out/test_speed2: $(OBJS_SPEED_2) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/test_speed3: $(OBJS_SPEED_3) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/test_speed5: $(OBJS_SPEED_5) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/test_gperf5: $(OBJS_GPERF_5) | out
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS_GPERF)

out/speed_hyperball2: $(OBJS_SPEED_HB_2) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/speed_hyperball3: $(OBJS_SPEED_HB_3) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/speed_hyperball5: $(OBJS_SPEED_HB_5) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/gperf_hyperball2: $(OBJS_GPERF_HB_2) | out
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS_GPERF)

out/gperf_hyperball3: $(OBJS_GPERF_HB_3) | out
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS_GPERF)

out/gperf_hyperball5: $(OBJS_GPERF_HB_5) | out
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS_GPERF)

out/get_gaussian_samples: $(OBJS_GAUSSIAN) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/%_sp_2.o: %.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=2 $(DEPFLAGS) -c $< -o $@

out/%_sp_3.o: %.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=3 $(DEPFLAGS) -c $< -o $@

out/%_sp_5.o: %.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=5 $(DEPFLAGS) -c $< -o $@

out/%_sp_2.o: %.S | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=2 -c $< -o $@

out/%_sp_3.o: %.S | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=3 -c $< -o $@

out/%_sp_5.o: %.S | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=5 -c $< -o $@

out/%_gf_2.o: %.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=2 $(DEPFLAGS) -c $< -o $@

out/%_gf_3.o: %.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=3 $(DEPFLAGS) -c $< -o $@

out/%_gf_5.o: %.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=5 $(DEPFLAGS) -c $< -o $@

out/%_gf_2.o: %.S | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=2 -c $< -o $@

out/%_gf_3.o: %.S | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=3 -c $< -o $@

out/%_gf_5.o: %.S | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=5 -c $< -o $@

out/%_gaussian.o: %.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=2 $(DEPFLAGS) -c $< -o $@

out/%_gaussian.o: %.S | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=2 -c $< -o $@

-include $(wildcard out/*.d)
