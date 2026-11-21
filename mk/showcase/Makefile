# Showcase ROM and HOST suite. Included by the top-level Makefile after SHOWCASE_FLAGS.
SHOWCASE_FLAGS += -DSC64_TYPED_UNIT_CHUNK_READER

SHOWCASE_TERRAN_BUILDING_WORDS ?= 0
ifneq ($(SHOWCASE_TERRAN_BUILDING_WORDS),0)
ifneq ($(SHOWCASE_TERRAN_BUILDING_WORDS),1)
$(error SHOWCASE_TERRAN_BUILDING_WORDS must be 0 or 1)
endif
endif
SHOWCASE_BUILDING_WORD_DIR := build/generated/showcase-terran-base-building-words
SHOWCASE_BUILDING_WORD_BANK := $(SHOWCASE_BUILDING_WORD_DIR)/sc64_building_words_generated.h
SHOWCASE_BUILDING_WORD_INPUTS := $(if $(filter 1,$(SHOWCASE_TERRAN_BUILDING_WORDS)),$(SHOWCASE_BUILDING_WORD_BANK))
SHOWCASE_BUILDING_WORD_FLAGS := -DSC64_BUILDING_WORDS_PROBE -I$(SHOWCASE_BUILDING_WORD_DIR)
$(SHOWCASE_BUILDING_WORD_BANK): .local/sc64/p7/generated/sc64_race_sprites_generated.h tools/sc64_building_words.py tools/sc64_unit_facing.py | build
	mkdir -p $(dir $@)
	$(PYTHON) tools/sc64_building_words.py $< --frame command_center --frame barracks --frame supply_depot > $@.tmp
	mv $@.tmp $@

SHOWCASE_BUILDING_WORD_CONFIG := $(if $(shell test -f build/showcase-building-words-config && printf '%s\n' '$(SHOWCASE_TERRAN_BUILDING_WORDS)' | cmp -s - build/showcase-building-words-config && printf matched),,force-showcase-building-words-rebuild)
.PHONY: force-showcase-building-words-config force-showcase-building-words-rebuild
force-showcase-building-words-config:
force-showcase-building-words-rebuild:
build/showcase-building-words-config: force-showcase-building-words-config | build
	@printf '%s\n' '$(SHOWCASE_TERRAN_BUILDING_WORDS)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

build/starcraft-showcase.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(SHOWCASE_BUILDING_WORD_INPUTS) common/gba.ld Makefile mk/combat-perf/Makefile mk/showcase/Makefile build/showcase-building-words-config $(SHOWCASE_BUILDING_WORD_CONFIG) | build
	$(CC) $(if $(SHOWCASE_BUILDING_WORD_INPUTS),$(SHOWCASE_BUILDING_WORD_FLAGS)) $(CFLAGS) $(SHOWCASE_FLAGS) $(CANONICAL_DEFENSE_FLAGS) $(CANONICAL_TARGET_INDEX_FLAGS) $(CANONICAL_TARGET_POLICY_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-showcase.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Showcase-Transport1-GBA.gba: build/starcraft-showcase.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC SHOWCASE' SCSH
.PHONY: showcase verify-showcase test-sc64-showcase test-sc64-showcase-front
showcase: build/StarCraft-Showcase-Transport1-GBA.gba
verify-showcase: build/StarCraft-Showcase-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --showcase

.PHONY: test-sc64-showcase-terran-building-words-pixels
build/test-sc64-showcase-terran-building-words-pixels: tests/sc64_building_words_pixels.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(SHOWCASE_BUILDING_WORD_BANK) common/gba.c $(COMMON_HEADERS) Makefile mk/showcase/Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_BUILDING_WORD_FLAGS) -DSC64_TERRAN_BASE_BUILDING_WORDS_EXPECTED $(SHOWCASE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
test-sc64-showcase-terran-building-words-pixels: build/test-sc64-showcase-terran-building-words-pixels
	./$<

# The indexed build drops packed word sprites, so it also drops the walk-word
# poses that depend on them (sc64_unit_facing.h rejects one without the other).
build/test-sc64-showcase-front: tests/sc64_frontend.c starcraft/sc64_frontend.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DSC64_SHOWCASE_ENABLED -Icommon $< -o $@
build/test-sc64-showcase-game: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-showcase-game-packed: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) -DSC64_UNIT_WORD_SPRITES -DGBA_SCENE_CLIP $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-showcase-game-indexed: tests/sc64_showcase_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(filter-out -DSC64_UNIT_WORD_SPRITES $(CANONICAL_WALK_WORDS_FLAGS),$(SHOWCASE_FLAGS)) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
test-sc64-showcase-front: build/test-sc64-showcase-front
	./build/test-sc64-showcase-front
test-starcraft: test-sc64-showcase-front
test-sc64-showcase: test-sc64-showcase-front build/test-sc64-showcase-game build/test-sc64-showcase-game-packed build/test-sc64-showcase-game-indexed
	./build/test-sc64-showcase-game
	./build/test-sc64-showcase-game-packed
	./build/test-sc64-showcase-game-indexed

# Separate experimental cartridge and journal; standard Showcase stays unchanged.
include mk/carrier-release/flags/Makefile
SHOWCASE_CARRIER_PROBE_FLAGS = $(SC64_CARRIER_RELEASE_FLAGS)
SHOWCASE_CARRIER_FACT_DIR = build/showcase-carrier-facts
SHOWCASE_CARRIER_FACT_HEADER = $(SHOWCASE_CARRIER_FACT_DIR)/sc64_fact_tables_generated.h
SHOWCASE_CARRIER_FLAGS = $(SHOWCASE_FLAGS) $(SHOWCASE_CARRIER_PROBE_FLAGS) -DSC64_FACT_TABLES -I$(SHOWCASE_CARRIER_FACT_DIR)
SHOWCASE_CARRIER_DEPS = $(COMMON_HEADERS) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) $(UNIT_FACING_INPUTS) $(COMMAND_ICON_HEADER) $(TANK_SOURCE_HEADER) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) mk/showcase/Makefile mk/carrier-release/flags/Makefile Makefile
$(SHOWCASE_CARRIER_FACT_HEADER): tools/sc64_fact_tables_gen.c $(SHOWCASE_CARRIER_DEPS) | build
	mkdir -p $(SHOWCASE_CARRIER_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $(SHOWCASE_CARRIER_PROBE_FLAGS) $< -o $(SHOWCASE_CARRIER_FACT_DIR)/gen
	$(SHOWCASE_CARRIER_FACT_DIR)/gen > $@.tmp
	mv $@.tmp $@

build/starcraft-showcase-carrier-probe.elf: $(COMMON) $(SHOWCASE_CARRIER_DEPS) $(SHOWCASE_CARRIER_FACT_HEADER) common/gba.ld | build
	$(CC) $(CFLAGS) $(SHOWCASE_CARRIER_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-showcase-carrier-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Showcase-Carrier-Probe-GBA.gba: build/starcraft-showcase-carrier-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC CARRIER' SCCP
.PHONY: showcase-carrier-probe test-sc64-showcase-carrier
showcase-carrier-probe: build/StarCraft-Showcase-Carrier-Probe-GBA.gba

SHOWCASE_CARRIER_TEST_SOURCES = tests/sc64_showcase_carrier_journal.c tests/sc64_yamato_host_sram_shim.c
build/test-sc64-showcase-carrier-predecessor: $(SHOWCASE_CARRIER_TEST_SOURCES) $(SHOWCASE_CARRIER_DEPS) common/gba.c | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_FLAGS) $(SHOWCASE_CARRIER_TEST_SOURCES) $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-showcase-carrier-journal: $(SHOWCASE_CARRIER_TEST_SOURCES) $(SHOWCASE_CARRIER_DEPS) $(SHOWCASE_CARRIER_FACT_HEADER) common/gba.c | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(SHOWCASE_CARRIER_FLAGS) $(SHOWCASE_CARRIER_TEST_SOURCES) $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
test-sc64-showcase-carrier: build/test-sc64-showcase-carrier-predecessor build/test-sc64-showcase-carrier-journal
	set -eu; fixture="$$(mktemp -d build/showcase-carrier-journal.XXXXXX)"; trap 'rm -rf "$$fixture"' EXIT; \
		./build/test-sc64-showcase-carrier-predecessor "$$fixture/predecessor.sav"; \
		./build/test-sc64-showcase-carrier-journal "$$fixture/predecessor.sav" "$$fixture/carrier.sav"; \
		./build/test-sc64-showcase-carrier-journal "$$fixture/carrier.sav"
