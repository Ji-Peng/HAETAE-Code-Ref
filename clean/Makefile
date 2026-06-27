CC = gcc
LD = gcc
LDFLAGS =
LIBS_GPERF = -lprofiler
RM = /bin/rm

DEPFLAGS = -MT $@ -MMD -MP -MF out/$*.d
CFLAGS = -Wall -Wextra -Wpedantic -Wmissing-prototypes -Wredundant-decls \
	-Wshadow -Wpointer-arith -O3 -fomit-frame-pointer -I../avx2/common -I./ -Itest
CFLAGS_GPERF = -Wall -Wextra -Wpedantic -Wredundant-decls \
	-Wshadow -Wpointer-arith -O2 -fno-omit-frame-pointer -fno-inline -fno-lto -g \
	-I../avx2/common -I./ -Itest

COMMON_DIR = ../avx2/common
COMMON_SRCS = randombytes.c fips202.c
COMMON_HEADERS = $(COMMON_DIR)/randombytes.h $(COMMON_DIR)/fips202.h

SOURCES = decompose.c encoding.c fft.c fixpoint.c ntt.c packing.c poly.c polyfix.c polymat.c polyvec.c reduce.c sampler.c sign.c symmetric-shake.c
HEADERS = decompose.h encoding.h fft.h fixpoint.h ntt.h packing.h poly.h polyfix.h polymat.h polyvec.h reduce.h sampler.h sign.h symmetric.h rans_byte.h

SRCS_SPEED = test/test_speed.c test/speed_print.c test/cpucycles.c
SRCS_SPEED_HB = test/speed_hyperball.c test/speed_print.c test/cpucycles.c
SRCS_GPERF_HB = test/gperf_hyperball.c
SRCS_COLLECT = test/collect_signatures.c
SRCS_COLLECT_PUBLIC = test/collect_public_view.c
SRCS_COLLECT_HINT = test/collect_hint_view.c
SRCS_COLLECT_HINT_PK = test/collect_hint_pk_view.c
SRCS_COLLECT_PARALLEL = test/collect_parallel.c
COMMON_OBJS_SPEED_2 = $(addprefix out/common_, $(COMMON_SRCS:.c=_sp_2.o))
COMMON_OBJS_SPEED_3 = $(addprefix out/common_, $(COMMON_SRCS:.c=_sp_3.o))
COMMON_OBJS_SPEED_5 = $(addprefix out/common_, $(COMMON_SRCS:.c=_sp_5.o))
COMMON_OBJS_GPERF_2 = $(addprefix out/common_, $(COMMON_SRCS:.c=_gf_2.o))
COMMON_OBJS_GPERF_3 = $(addprefix out/common_, $(COMMON_SRCS:.c=_gf_3.o))
COMMON_OBJS_GPERF_5 = $(addprefix out/common_, $(COMMON_SRCS:.c=_gf_5.o))

OBJS_SPEED_2 = $(addprefix out/, $(SOURCES:.c=_sp_2.o)) $(COMMON_OBJS_SPEED_2) $(addprefix out/, $(SRCS_SPEED:.c=_sp_2.o))
OBJS_SPEED_3 = $(addprefix out/, $(SOURCES:.c=_sp_3.o)) $(COMMON_OBJS_SPEED_3) $(addprefix out/, $(SRCS_SPEED:.c=_sp_3.o))
OBJS_SPEED_5 = $(addprefix out/, $(SOURCES:.c=_sp_5.o)) $(COMMON_OBJS_SPEED_5) $(addprefix out/, $(SRCS_SPEED:.c=_sp_5.o))

OBJS_SPEED_HB_2 = $(addprefix out/, $(SOURCES:.c=_sp_2.o)) $(COMMON_OBJS_SPEED_2) $(addprefix out/, $(SRCS_SPEED_HB:.c=_sp_2.o))
OBJS_SPEED_HB_3 = $(addprefix out/, $(SOURCES:.c=_sp_3.o)) $(COMMON_OBJS_SPEED_3) $(addprefix out/, $(SRCS_SPEED_HB:.c=_sp_3.o))
OBJS_SPEED_HB_5 = $(addprefix out/, $(SOURCES:.c=_sp_5.o)) $(COMMON_OBJS_SPEED_5) $(addprefix out/, $(SRCS_SPEED_HB:.c=_sp_5.o))

OBJS_GPERF_HB_2 = $(addprefix out/, $(SOURCES:.c=_gf_2.o)) $(COMMON_OBJS_GPERF_2) $(addprefix out/, $(SRCS_GPERF_HB:.c=_gf_2.o))
OBJS_GPERF_HB_3 = $(addprefix out/, $(SOURCES:.c=_gf_3.o)) $(COMMON_OBJS_GPERF_3) $(addprefix out/, $(SRCS_GPERF_HB:.c=_gf_3.o))
OBJS_GPERF_HB_5 = $(addprefix out/, $(SOURCES:.c=_gf_5.o)) $(COMMON_OBJS_GPERF_5) $(addprefix out/, $(SRCS_GPERF_HB:.c=_gf_5.o))

OBJS_COLLECT_2 = $(addprefix out/, $(SOURCES:.c=_sp_2.o)) $(COMMON_OBJS_SPEED_2) $(addprefix out/, $(SRCS_COLLECT:.c=_sp_2.o))
OBJS_COLLECT_3 = $(addprefix out/, $(SOURCES:.c=_sp_3.o)) $(COMMON_OBJS_SPEED_3) $(addprefix out/, $(SRCS_COLLECT:.c=_sp_3.o))
OBJS_COLLECT_5 = $(addprefix out/, $(SOURCES:.c=_sp_5.o)) $(COMMON_OBJS_SPEED_5) $(addprefix out/, $(SRCS_COLLECT:.c=_sp_5.o))
OBJS_COLLECT_PUBLIC_2 = $(addprefix out/, $(SOURCES:.c=_sp_2.o)) $(COMMON_OBJS_SPEED_2) $(addprefix out/, $(SRCS_COLLECT_PUBLIC:.c=_sp_2.o))
OBJS_COLLECT_PUBLIC_3 = $(addprefix out/, $(SOURCES:.c=_sp_3.o)) $(COMMON_OBJS_SPEED_3) $(addprefix out/, $(SRCS_COLLECT_PUBLIC:.c=_sp_3.o))
OBJS_COLLECT_PUBLIC_5 = $(addprefix out/, $(SOURCES:.c=_sp_5.o)) $(COMMON_OBJS_SPEED_5) $(addprefix out/, $(SRCS_COLLECT_PUBLIC:.c=_sp_5.o))
OBJS_COLLECT_HINT_2 = $(addprefix out/, $(SOURCES:.c=_sp_2.o)) $(COMMON_OBJS_SPEED_2) $(addprefix out/, $(SRCS_COLLECT_HINT:.c=_sp_2.o))
OBJS_COLLECT_HINT_3 = $(addprefix out/, $(SOURCES:.c=_sp_3.o)) $(COMMON_OBJS_SPEED_3) $(addprefix out/, $(SRCS_COLLECT_HINT:.c=_sp_3.o))
OBJS_COLLECT_HINT_5 = $(addprefix out/, $(SOURCES:.c=_sp_5.o)) $(COMMON_OBJS_SPEED_5) $(addprefix out/, $(SRCS_COLLECT_HINT:.c=_sp_5.o))
OBJS_COLLECT_HINT_PK_2 = $(addprefix out/, $(SOURCES:.c=_sp_2.o)) $(COMMON_OBJS_SPEED_2) $(addprefix out/, $(SRCS_COLLECT_HINT_PK:.c=_sp_2.o))
OBJS_COLLECT_PARALLEL_2 = $(addprefix out/, $(SOURCES:.c=_sp_2.o)) $(COMMON_OBJS_SPEED_2) $(addprefix out/, $(SRCS_COLLECT_PARALLEL:.c=_sp_2.o))
OBJS_COLLECT_HINT_PK_3 = $(addprefix out/, $(SOURCES:.c=_sp_3.o)) $(COMMON_OBJS_SPEED_3) $(addprefix out/, $(SRCS_COLLECT_HINT_PK:.c=_sp_3.o))
OBJS_COLLECT_HINT_PK_5 = $(addprefix out/, $(SOURCES:.c=_sp_5.o)) $(COMMON_OBJS_SPEED_5) $(addprefix out/, $(SRCS_COLLECT_HINT_PK:.c=_sp_5.o))

.PHONY: all run_speed run_speed_hyperball clean collect collect_public collect_hint collect_hint_pk

all: out/test_speed2 out/test_speed3 out/test_speed5

speed_hyperball: out/speed_hyperball2 out/speed_hyperball3 out/speed_hyperball5

gperf_hyperball: out/gperf_hyperball2 out/gperf_hyperball3 out/gperf_hyperball5

collect: out/collect2 out/collect3 out/collect5

collect_public: out/collect_public2 out/collect_public3 out/collect_public5

collect_hint: out/collect_hint2 out/collect_hint3 out/collect_hint5

collect_hint_pk: out/collect_hint_pk2 out/collect_hint_pk3 out/collect_hint_pk5

run_speed: all
	./out/test_speed2 > speed.txt
	./out/test_speed3 >> speed.txt
	./out/test_speed5 >> speed.txt

run_speed_hyperball: speed_hyperball
	./out/speed_hyperball2 > speed_hyperball.txt
	./out/speed_hyperball3 >> speed_hyperball.txt
	./out/speed_hyperball5 >> speed_hyperball.txt

out:
	@mkdir -p out
	@mkdir -p out/test

out/test_speed2: $(OBJS_SPEED_2) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/test_speed3: $(OBJS_SPEED_3) | out
	$(LD) $(LDFLAGS) $^ -o $@

out/test_speed5: $(OBJS_SPEED_5) | out
	$(LD) $(LDFLAGS) $^ -o $@

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

out/collect2: $(OBJS_COLLECT_2) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect3: $(OBJS_COLLECT_3) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect5: $(OBJS_COLLECT_5) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_public2: $(OBJS_COLLECT_PUBLIC_2) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_public3: $(OBJS_COLLECT_PUBLIC_3) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_public5: $(OBJS_COLLECT_PUBLIC_5) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_hint2: $(OBJS_COLLECT_HINT_2) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_hint3: $(OBJS_COLLECT_HINT_3) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_hint5: $(OBJS_COLLECT_HINT_5) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_hint_pk2: $(OBJS_COLLECT_HINT_PK_2) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_parallel2: $(OBJS_COLLECT_PARALLEL_2) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_hint_pk3: $(OBJS_COLLECT_HINT_PK_3) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/collect_hint_pk5: $(OBJS_COLLECT_HINT_PK_5) | out
	$(LD) $(LDFLAGS) $^ -o $@ -lm

out/%_sp_2.o: %.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=2 $(DEPFLAGS) -c $< -o $@

out/%_sp_3.o: %.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=3 $(DEPFLAGS) -c $< -o $@

out/%_sp_5.o: %.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=5 $(DEPFLAGS) -c $< -o $@

out/%_gf_2.o: %.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=2 $(DEPFLAGS) -c $< -o $@

out/%_gf_3.o: %.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=3 $(DEPFLAGS) -c $< -o $@

out/%_gf_5.o: %.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=5 $(DEPFLAGS) -c $< -o $@

out/common_%_sp_2.o: $(COMMON_DIR)/%.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=2 $(DEPFLAGS) -c $< -o $@

out/common_%_sp_3.o: $(COMMON_DIR)/%.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=3 $(DEPFLAGS) -c $< -o $@

out/common_%_sp_5.o: $(COMMON_DIR)/%.c | out
	$(CC) $(CFLAGS) -DHAETAE_MODE=5 $(DEPFLAGS) -c $< -o $@

out/common_%_gf_2.o: $(COMMON_DIR)/%.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=2 $(DEPFLAGS) -c $< -o $@

out/common_%_gf_3.o: $(COMMON_DIR)/%.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=3 $(DEPFLAGS) -c $< -o $@

out/common_%_gf_5.o: $(COMMON_DIR)/%.c | out
	$(CC) $(CFLAGS_GPERF) -DHAETAE_MODE=5 $(DEPFLAGS) -c $< -o $@

clean:
	-rm -rf out/
	-$(RM) -f speed.txt speed_hyperball.txt

-include $(wildcard out/*.d)
