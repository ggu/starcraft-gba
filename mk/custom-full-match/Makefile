# SPDX-License-Identifier: GPL-3.0-or-later
# Included late from mk/skirmish-controller/Makefile (the top-level Makefile is
# at its size limit). HOST authors for whole Custom Game matches, each built
# from the same product vector as its race cartridge so a held-key trace
# written here replays on that ARM ROM. Unsanitized -O2: the author plays
# whole matches many times while a route is tuned.
# The cartridge ELFs also receive target-specific CFLAGS (collision buckets,
# contact axis, shared sight rows, the Melee lobby row); repeat them here.
# tools/sc64_custom_full_match.py refuses a trace whose lobby or checkpoints
# disagree with the ROM, which is how a missed flag shows up.
CUSTOM_FULL_MATCH_ELF_FLAGS = -DSC64_PERSISTENT_COLLISION_BUCKETS $(CANONICAL_CONTACT_ROM_AXIS_FLAGS) $(NATIVE_MELEE_START_FLAGS)
CUSTOM_FULL_MATCH_SIGHT_FLAGS = -DSC64_SHARED_SIGHT_PACKED_ROWS -DSC64_SHARED_SIGHT_SPARSE_ROWS -I$(SHARED_SIGHT_DIR)
CUSTOM_FULL_MATCH_SRC ?= tests/sc64_custom_full_match.c
CUSTOM_FULL_MATCH_HOST_FLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -D_FORTIFY_SOURCE=0 -DGBA_HOST -Icommon
CUSTOM_FULL_MATCH_FLAGS_terran = $(TERRAN_CARRIER_FLAGS) $(if $(TERRAN_BUILDING_WORD_INPUTS),$(TERRAN_BUILDING_WORD_FLAGS)) $(TERRAN_ROM_FLAGS) $(CANONICAL_DEFENSE_FLAGS) $(CANONICAL_TARGET_INDEX_FLAGS) $(CANONICAL_TARGET_POLICY_FLAGS) $(CUSTOM_FULL_MATCH_ELF_FLAGS) $(CUSTOM_FULL_MATCH_SIGHT_FLAGS)
CUSTOM_FULL_MATCH_SOURCES_terran = $(filter-out starcraft/game.c,$(TERRAN_ARM_SOURCES))
CUSTOM_FULL_MATCH_INPUTS_terran = $(TERRAN_FULL_INPUTS) $(TERRAN_BUILDING_WORD_INPUTS)
CUSTOM_FULL_MATCH_FLAGS_zerg = $(Z10_TERRAN_BUILDING_WORD_INCLUDE) $(Z10_FACT_FLAGS) $(Z10_CARRIER_FLAGS) $(Z10_PADDED_IMPACT_FLAGS) $(ZERG_Z10_RELEASE_FLAGS) $(Z10_BUILDING_WORDS_FLAGS) $(CANONICAL_TARGET_INDEX_FLAGS) $(CANONICAL_TARGET_POLICY_FLAGS) $(CUSTOM_FULL_MATCH_ELF_FLAGS)
CUSTOM_FULL_MATCH_SOURCES_zerg = $(filter-out starcraft/game.c,$(Z10_ARM_SOURCES))
CUSTOM_FULL_MATCH_INPUTS_zerg = $(ZERG_Z10_CANONICAL_INPUTS) $(if $(Z10_FACT_FLAGS),$(Z10_FACT_HEADER))
CUSTOM_FULL_MATCH_FLAGS_protoss = $(P10_BUILDING_WORD_FLAGS) $(P10_PADDED_IMPACT_FLAGS) $(PROTOSS_P10_FLAGS) $(CANONICAL_DEFENSE_FLAGS) $(CANONICAL_TARGET_INDEX_FLAGS) $(CANONICAL_TARGET_POLICY_FLAGS) $(CUSTOM_FULL_MATCH_ELF_FLAGS) $(CUSTOM_FULL_MATCH_SIGHT_FLAGS)
CUSTOM_FULL_MATCH_SOURCES_protoss = $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
CUSTOM_FULL_MATCH_INPUTS_protoss = $(PROTOSS_P10_INPUTS) $(P10_BUILDING_WORD_HEADER)
# Each author depends on its cartridge ELF: that brings in every generated
# header and configuration stamp the product vector needs, and rebuilds the
# author whenever the cartridge is rebuilt.
CUSTOM_FULL_MATCH_ELF_terran := build/starcraft-terran.elf
CUSTOM_FULL_MATCH_ELF_zerg := build/starcraft-zerg-z10.elf
CUSTOM_FULL_MATCH_ELF_protoss := build/starcraft-protoss-p10.elf
define CUSTOM_FULL_MATCH_AUTHOR
build/test-sc64-custom-full-match-$(1): $$(CUSTOM_FULL_MATCH_SRC) tests/sc64_visible_mineral_cursor.h $$(CUSTOM_FULL_MATCH_ELF_$(1)) $$(CUSTOM_FULL_MATCH_SOURCES_$(1)) $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(CUSTOM_FULL_MATCH_INPUTS_$(1)) common/gba.c $$(COMMON_HEADERS) mk/custom-full-match/Makefile | build
	$$(CLANG) $$(CUSTOM_FULL_MATCH_HOST_FLAGS) $$(CUSTOM_FULL_MATCH_FLAGS_$(1)) $$< common/gba.c $$(CUSTOM_FULL_MATCH_SOURCES_$(1)) -o $$@
endef
$(foreach product,terran zerg protoss,$(eval $(call CUSTOM_FULL_MATCH_AUTHOR,$(product))))
.PHONY: sc64-custom-full-match-authors
sc64-custom-full-match-authors: $(addprefix build/test-sc64-custom-full-match-,terran zerg protoss)

# Author every qualified lobby and replay it on its cartridge with physical
# buttons: the whole tape, a fresh-process continuation of its mid-battle
# pause save, and a fresh-process reload of the RESULT save. Needs the
# prepared inputs and the port-free mGBA backend:
#   make check-sc64-custom-full-matches SC64_HEADLESS_REPLAY=<sc64-headless-replay>
# CUSTOM_FULL_MATCH_CASES='terran:0:zpt' narrows the run; the output folder
# must not exist yet.
CUSTOM_FULL_MATCH_OUT ?= .local/sc64/custom-full-matches
CUSTOM_FULL_MATCH_JOBS ?= 3
CUSTOM_FULL_MATCH_ROMS := build/StarCraft-Terran-Transport1-GBA.gba build/StarCraft-Zerg-Z10-Transport1-GBA.gba build/StarCraft-Protoss-P10-Transport1-GBA.gba
.PHONY: check-sc64-custom-full-matches
check-sc64-custom-full-matches: sc64-custom-full-match-authors $(CUSTOM_FULL_MATCH_ROMS) tools/sc64_custom_full_match.py
	@test -n "$(SC64_HEADLESS_REPLAY)" || { echo "set SC64_HEADLESS_REPLAY to the port-free mGBA backend"; exit 2; }
	$(PYTHON) tools/sc64_custom_full_match.py matrix --runner $(SC64_HEADLESS_REPLAY) --output $(CUSTOM_FULL_MATCH_OUT) --jobs $(CUSTOM_FULL_MATCH_JOBS) $(CUSTOM_FULL_MATCH_CASES)
# Hash the header selected by each product's real include order every time a
# product is requested. Only a changed hash updates the stamp and relinks.
SKIRMISH_PACK_STAMP_DIR ?= build
.PHONY: force-skirmish-pack-stamp
force-skirmish-pack-stamp:

define SKIRMISH_PACK_STAMP
$(SKIRMISH_PACK_STAMP_DIR)/skirmish-pack-$(1).stamp: force-skirmish-pack-stamp | build
	@set -e; input=$$$$(mktemp "$$@.input.XXXXXX"); digest=$$$$(mktemp "$$@.digest.XXXXXX"); \
	trap 'rm -f "$$$$input" "$$$$digest"' EXIT HUP INT TERM; \
	$$(CLANG) -E -P -x c $$(filter -I%,$(2)) -include starcraft/sc64_skirmish_pack_gate.h /dev/null > "$$$$input"; \
	$$(PYTHON) -c 'import hashlib,sys; print(hashlib.sha256(open(sys.argv[1], "rb").read()).hexdigest())' "$$$$input" > "$$$$digest"; \
	cmp -s "$$$$digest" "$$@" || mv "$$$$digest" "$$@"
endef
$(eval $(call SKIRMISH_PACK_STAMP,terran,$$(TERRAN_CARRIER_FLAGS) $$(if $$(TERRAN_BUILDING_WORD_INPUTS),$$(TERRAN_BUILDING_WORD_FLAGS)) $$(CFLAGS) $$(TERRAN_ROM_FLAGS)))
$(eval $(call SKIRMISH_PACK_STAMP,zerg,$$(Z10_TERRAN_BUILDING_WORD_INCLUDE) $$(Z10_FACT_FLAGS) $$(Z10_CARRIER_FLAGS) $$(Z10_PADDED_IMPACT_FLAGS) $$(CFLAGS) $$(ZERG_Z10_RELEASE_FLAGS) $$(Z10_BUILDING_WORDS_FLAGS)))
$(eval $(call SKIRMISH_PACK_STAMP,protoss,$$(P10_BUILDING_WORD_FLAGS) $$(CFLAGS) $$(P10_PADDED_IMPACT_FLAGS) $$(PROTOSS_P10_FLAGS)))
$(eval $(call SKIRMISH_PACK_STAMP,showcase,$$(if $$(SHOWCASE_BUILDING_WORD_INPUTS),$$(SHOWCASE_BUILDING_WORD_FLAGS)) $$(CFLAGS) $$(SHOWCASE_FLAGS)))
build/starcraft-terran.elf: $(SKIRMISH_PACK_STAMP_DIR)/skirmish-pack-terran.stamp
build/starcraft-zerg-z10.elf: $(SKIRMISH_PACK_STAMP_DIR)/skirmish-pack-zerg.stamp
build/starcraft-protoss-p10.elf: $(SKIRMISH_PACK_STAMP_DIR)/skirmish-pack-protoss.stamp
build/starcraft-showcase.elf: $(SKIRMISH_PACK_STAMP_DIR)/skirmish-pack-showcase.stamp
