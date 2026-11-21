# SPDX-License-Identifier: GPL-3.0-or-later
include mk/carrier-release/flags/Makefile
# The normal Terran cartridge uses the shared compact Custom Game journal.
TERRAN_CARRIER_RELEASE ?= 1
ifneq ($(TERRAN_CARRIER_RELEASE),0)
ifneq ($(TERRAN_CARRIER_RELEASE),1)
$(error TERRAN_CARRIER_RELEASE must be 0 or 1)
endif
endif
TERRAN_CARRIER_FLAGS := $(if $(filter 1,$(TERRAN_CARRIER_RELEASE)),$(SC64_CARRIER_RELEASE_FLAGS))
TERRAN_CARRIER_SOURCES := $(if $(TERRAN_CARRIER_FLAGS),$(P7_INTERCEPTOR_FLIGHT_SOURCES))
TERRAN_PROJECTILE_SOURCES := $(filter starcraft/sc64_interceptor_%.c,$(STARCRAFT))
TERRAN_ARM_SOURCES := $(filter-out $(TERRAN_PROJECTILE_SOURCES),$(STARCRAFT)) $(TERRAN_CARRIER_SOURCES) $(TERRAN_PROJECTILE_SOURCES)
# Optional packed art for the three stock Terran base structures.
TERRAN_BUILDING_WORDS ?= $(CANONICAL_TERRAN_BUILDING_WORDS)
ifneq ($(TERRAN_BUILDING_WORDS),0)
ifneq ($(TERRAN_BUILDING_WORDS),1)
$(error TERRAN_BUILDING_WORDS must be 0 or 1)
endif
endif
TERRAN_BUILDING_WORD_DIR := build/generated/terran-base-building-words
TERRAN_BUILDING_WORD_BANK := $(TERRAN_BUILDING_WORD_DIR)/sc64_building_words_generated.h
TERRAN_BUILDING_WORD_INPUTS := $(if $(and $(filter 1,$(TERRAN_BUILDING_WORDS)),$(call sc64_can_pack_building_words,$(CFLAGS) $(TERRAN_ROM_FLAGS))),$(TERRAN_BUILDING_WORD_BANK))
TERRAN_BUILDING_WORD_FLAGS := -DSC64_BUILDING_WORDS_PROBE -I$(TERRAN_BUILDING_WORD_DIR)

$(TERRAN_BUILDING_WORD_BANK): $(LATE_TERRAN_STAGE)/sc64_race_sprites_generated.h tools/sc64_building_words.py tools/sc64_unit_facing.py | build
	mkdir -p $(dir $@)
	$(PYTHON) tools/sc64_building_words.py $< --frame command_center --frame barracks --frame supply_depot > $@.tmp
	mv $@.tmp $@

TERRAN_NORMAL_OPTIONS := $(TERRAN_CARRIER_RELEASE) $(if $(TERRAN_BUILDING_WORD_INPUTS),1,0)
TERRAN_NORMAL_CONFIG := $(if $(shell test -f build/terran-normal-config && printf '%s\n' '$(TERRAN_NORMAL_OPTIONS)' | cmp -s - build/terran-normal-config && printf matched),,force-terran-normal-rebuild)
.PHONY: force-terran-normal-config force-terran-normal-rebuild
force-terran-normal-config:
force-terran-normal-rebuild:
build/terran-normal-config: force-terran-normal-config | build
	@printf '%s\n' '$(TERRAN_NORMAL_OPTIONS)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

build/starcraft-terran.elf: $(COMMON) $(COMMON_HEADERS) $(TERRAN_ARM_SOURCES) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(TERRAN_BUILDING_WORD_INPUTS) common/gba.h common/gba.ld Makefile mk/combat-perf/Makefile mk/carrier-release/flags/Makefile mk/terran-normal/Makefile build/terran-normal-config $(TERRAN_NORMAL_CONFIG) | build
	$(CC) $(TERRAN_CARRIER_FLAGS) $(if $(TERRAN_BUILDING_WORD_INPUTS),$(TERRAN_BUILDING_WORD_FLAGS)) $(CFLAGS) $(TERRAN_ROM_FLAGS) $(CANONICAL_DEFENSE_FLAGS) $(CANONICAL_TARGET_INDEX_FLAGS) $(CANONICAL_TARGET_POLICY_FLAGS) $(COMMON) $(TERRAN_ARM_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-terran.map -lgcc -o $@
	$(SIZE) $@

.PHONY: test-sc64-terran-building-words-pixels
build/test-sc64-terran-building-words-pixels: tests/sc64_building_words_pixels.c $(TERRAN_ARM_SOURCES) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(TANK_SOURCE_HEADER) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) $(TERRAN_BUILDING_WORD_BANK) common/gba.c $(COMMON_HEADERS) Makefile mk/terran-normal/Makefile | build
	$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_BUILDING_WORD_FLAGS) -DSC64_TERRAN_BASE_BUILDING_WORDS_EXPECTED $(TERRAN_CARRIER_FLAGS) $(TERRAN_ROM_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(TERRAN_ARM_SOURCES)) -o $@
test-sc64-terran-building-words-pixels: build/test-sc64-terran-building-words-pixels
	./$<
