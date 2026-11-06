# SPDX-License-Identifier: GPL-3.0-or-later
# Run explicitly under the shared native-build lock; all outputs stay ignored.
DIAMOND_BENCH_OUT ?= .local/diamond-bench
DIAMOND_BENCH_FLAGS = -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -g -std=c11 -ffreestanding -fno-builtin -fno-strict-aliasing -fno-common -ffunction-sections -fdata-sections -Wall -Wextra -Icommon -DGBA_SCENE_CLIP
DIAMOND_BENCH_SRC = common/start.s common/main.c common/gba.c common/memory.c common/audio_bank.S tests/sc64-diamond-placement/bench.c
DIAMOND_BENCH_LINK = -mcpu=arm7tdmi -mthumb -mthumb-interwork -nostdlib -Tcommon/gba.ld -Wl,--gc-sections
.PHONY: diamond-bench
diamond-bench:
	mkdir -p $(DIAMOND_BENCH_OUT)
	arm-none-eabi-gcc $(DIAMOND_BENCH_FLAGS) -fstack-usage $(DIAMOND_BENCH_SRC) $(DIAMOND_BENCH_LINK) -Wl,-Map,$(DIAMOND_BENCH_OUT)/iwram.map -lgcc -o $(DIAMOND_BENCH_OUT)/iwram.elf
	arm-none-eabi-gcc $(DIAMOND_BENCH_FLAGS) -DGBA_DIAMOND_ROM -fstack-usage $(DIAMOND_BENCH_SRC) $(DIAMOND_BENCH_LINK) -Wl,-Map,$(DIAMOND_BENCH_OUT)/rom.map -lgcc -o $(DIAMOND_BENCH_OUT)/rom.elf
	arm-none-eabi-objcopy -O binary $(DIAMOND_BENCH_OUT)/iwram.elf $(DIAMOND_BENCH_OUT)/iwram.gba
	arm-none-eabi-objcopy -O binary $(DIAMOND_BENCH_OUT)/rom.elf $(DIAMOND_BENCH_OUT)/rom.gba
	python3 tools/fix_rom.py $(DIAMOND_BENCH_OUT)/iwram.gba 'DIAM BENCH I' 'DBI1'
	python3 tools/fix_rom.py $(DIAMOND_BENCH_OUT)/rom.gba 'DIAM BENCH R' 'DBR1'
