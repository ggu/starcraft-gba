# SPDX-License-Identifier: GPL-3.0-or-later
CLANG ?= clang
PREFIX ?= arm-none-eabi-
.PHONY: test-gba-flash1m-integrated build-gba-flash1m-integrated
test-gba-flash1m-integrated:
	@mkdir -p build/flash1m-integrated
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/gba_flash1m_model.c common/gba_flash1m.c -o build/flash1m-integrated/transport
	./build/flash1m-integrated/transport
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/gba_flash1m_journal_model.c common/gba_flash1m.c -o build/flash1m-integrated/journal
	./build/flash1m-integrated/journal

# Standalone ROMs only. Seed creation and mGBA runners are private test tools.
build-gba-flash1m-integrated:
	@mkdir -p build/flash1m-integrated
	$(PREFIX)gcc -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -std=c11 -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -nostdlib -Tcommon/gba.ld -Wl,--gc-sections common/start.s common/memory.c common/gba_flash1m.c tests/gba_flash1m_journal_rom.c -o build/flash1m-integrated/write.elf
	$(PREFIX)objcopy -O binary build/flash1m-integrated/write.elf build/flash1m-integrated/write.gba
	python3 tools/fix_rom.py build/flash1m-integrated/write.gba 'FLASH TEST' X1MT
	$(PREFIX)gcc -mcpu=arm7tdmi -mthumb -mthumb-interwork -O2 -std=c11 -ffreestanding -fno-builtin -ffunction-sections -fdata-sections -nostdlib -Tcommon/gba.ld -Wl,--gc-sections -DFLASH_JOURNAL_READ_ONLY common/start.s common/memory.c common/gba_flash1m.c tests/gba_flash1m_journal_rom.c -o build/flash1m-integrated/read.elf
	$(PREFIX)objcopy -O binary build/flash1m-integrated/read.elf build/flash1m-integrated/read.gba
	python3 tools/fix_rom.py build/flash1m-integrated/read.gba 'FLASH TEST' X1MT
