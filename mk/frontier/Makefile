# SPDX-License-Identifier: GPL-3.0-or-later
# Independently authored battle maps and the deterministic standalone bot AI.
ORIGINAL_SKIRMISH_FLAGS := -DSC64_SKIRMISH -DSC_ORIGINAL_SKIRMISH
.PHONY: original-skirmish test-original-skirmish
original-skirmish: build/Frontier-Skirmish-GBA.gba
	$(PYTHON) tools/verify_roms.py --frontier
build/frontier-skirmish.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.ld Makefile mk/frontier/Makefile | build
	$(CC) $(CFLAGS) $(OUTPOST_PRESENT_FLAGS) $(ORIGINAL_SKIRMISH_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/frontier-skirmish.map -lgcc -o $@
	$(SIZE) $@
build/Frontier-Skirmish-GBA.gba: build/frontier-skirmish.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'FRONTIER RTS' XFR1
build/test-original-skirmish-map: tests/original_skirmish_map.c starcraft/original_skirmish_map.h Makefile mk/frontier/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-original-skirmish-game: tests/original_skirmish_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c $(COMMON_HEADERS) Makefile mk/frontier/Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ORIGINAL_SKIRMISH_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
test-original-skirmish: build/test-original-skirmish-map build/test-original-skirmish-game
	./build/test-original-skirmish-map
	./build/test-original-skirmish-game
test: test-original-skirmish
