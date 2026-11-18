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

# Each cartridge has its own defaults and saves. All faction choices are shared.
define frontier_faction
.PHONY: frontier-$(1)
frontier-$(1): build/Frontier-$(2)-GBA.gba
	$(PYTHON) tools/verify_roms.py --frontier-$(1)
build/frontier-$(1).elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.ld Makefile mk/frontier/Makefile | build
	$(CC) $(CFLAGS) $(OUTPOST_PRESENT_FLAGS) $(ORIGINAL_SKIRMISH_FLAGS) -DSC_FRONTIER_FACTION=$(3) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/frontier-$(1).map -lgcc -o $$@
	$(SIZE) $$@
build/Frontier-$(2)-GBA.gba: build/frontier-$(1).elf tools/fix_rom.py
	$(OBJCOPY) -O binary $$< $$@
	$(PYTHON) tools/fix_rom.py $$@ '$(4)' $(5)
build/test-frontier-$(1): tests/frontier_factions_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c $(COMMON_HEADERS) Makefile mk/frontier/Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ORIGINAL_SKIRMISH_FLAGS) -DSC_FRONTIER_FACTION=$(3) $$< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $$@
endef

$(eval $(call frontier_faction,foundry,Foundry,0,FR FOUNDRY,XFF1))
$(eval $(call frontier_faction,rovers,Rovers,1,FR ROVERS,XFR2))
$(eval $(call frontier_faction,bulwark,Bulwark,2,FR BULWARK,XFB1))

.PHONY: frontier-factions test-frontier-factions
frontier-factions: frontier-foundry frontier-rovers frontier-bulwark
test-frontier-factions: build/test-frontier-foundry build/test-frontier-rovers build/test-frontier-bulwark
	./build/test-frontier-foundry
	./build/test-frontier-rovers
	./build/test-frontier-bulwark
test: test-frontier-factions

.PHONY: test-frontier-terrain-spans
build/test-frontier-terrain-spans: tests/frontier_terrain_spans.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c $(COMMON_HEADERS) Makefile mk/frontier/Makefile | build
	$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(ORIGINAL_SKIRMISH_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
test-frontier-terrain-spans: build/test-frontier-terrain-spans
	./build/test-frontier-terrain-spans
test: test-frontier-terrain-spans
