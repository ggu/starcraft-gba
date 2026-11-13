# Included by the top-level Makefile after the zerg-z8 rules, whose
# variables these recipes extend.

# Asset-free late-Zerg scheduler extensions and compact save state.
build/test-sc64-zerg-late-state: tests/sc64_zerg_late_state.c starcraft/sc64_zerg_late_state.h starcraft/sc64_zerg_late_triggers.h starcraft/sc64_z8_state.h starcraft/sc64_z8_triggers.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-zerg-late-state
test-sc64-zerg-late-state: build/test-sc64-zerg-late-state
	./build/test-sc64-zerg-late-state
test-starcraft: test-sc64-zerg-late-state

# Z8 ordinary-input qualification: the controller trace, its key tape, host
# replays (full, save prefix, fresh-process resume) and the same probe as ARM ROMs.
SC64_Z8_MISSION := 19
Z8_ARM_DIR := build/z8-arm
Z8_ARM_TRACE := $(Z8_ARM_DIR)/z8-controller.jsonl
Z8_TOOL = $(PYTHON) tools/sc64_p8_arm_replay.py --mission $(SC64_Z8_MISSION) --prefix PX
Z8_HOST_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-z8-campaign-run: tests/sc64_z8_campaign_run.c tests/sc64_zerg_late_controller.h $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z8_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z8_CANONICAL_FLAGS) $< -o $@
$(Z8_ARM_TRACE): build/test-sc64-z8-campaign-run
	mkdir -p $(Z8_ARM_DIR)
	./build/test-sc64-z8-campaign-run $@
.PHONY: test-sc64-z8-controller
test-sc64-z8-controller: $(Z8_ARM_TRACE)
$(Z8_ARM_DIR)/sc64_px_tape_generated.h: $(Z8_ARM_TRACE) tools/sc64_p8_arm_replay.py
	$(Z8_TOOL) tape $< $@
Z8_REPLAY_DEPS = tests/sc64_protoss_arm_replay.c $(Z8_ARM_DIR)/sc64_px_tape_generated.h $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z8_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile
build/test-sc64-z8-replay: $(Z8_REPLAY_DEPS) | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z8_CANONICAL_FLAGS) -I$(Z8_ARM_DIR) -DPX_MISSION=$(SC64_Z8_MISSION) $< -o $@
build/test-sc64-z8-replay-prefix: $(Z8_REPLAY_DEPS) | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z8_CANONICAL_FLAGS) -I$(Z8_ARM_DIR) -DPX_MISSION=$(SC64_Z8_MISSION) -DPX_REPLAY_PREFIX $< -o $@
build/test-sc64-z8-replay-resume: $(Z8_REPLAY_DEPS) | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z8_CANONICAL_FLAGS) -I$(Z8_ARM_DIR) -DPX_MISSION=$(SC64_Z8_MISSION) -DPX_REPLAY_RESUME $< -o $@
# The full tape must win; the prefix saves, and a fresh process that loads its
# SRAM image must match the full replay from the save on.
.PHONY: test-sc64-z8-replay
test-sc64-z8-replay: build/test-sc64-z8-replay build/test-sc64-z8-replay-prefix build/test-sc64-z8-replay-resume tools/sc64_p8_arm_replay.py
	./build/test-sc64-z8-replay > $(Z8_ARM_DIR)/host-full.txt
	./build/test-sc64-z8-replay-prefix $(Z8_ARM_DIR)/host-prefix.sav > $(Z8_ARM_DIR)/host-prefix.txt || true
	./build/test-sc64-z8-replay-resume $(Z8_ARM_DIR)/host-prefix.sav > $(Z8_ARM_DIR)/host-resume.txt
	$(Z8_TOOL) compare --from-tick $$(sed -n 's/^#define PX_TAPE_RESUME_TICK \([0-9]*\)u$$/\1/p' $(Z8_ARM_DIR)/sc64_px_tape_generated.h) $(Z8_ARM_DIR)/host-full.txt $(Z8_ARM_DIR)/host-resume.txt
.PHONY: test-sc64-zerg-z8
test-sc64-zerg-z8: test-zerg-z8 test-sc64-z8-controller test-sc64-z8-replay
define Z8_ARM_ROM
build/starcraft-zerg-z8-$(1).elf: $$(Z8_REPLAY_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(ZERG_Z8_CANONICAL_FLAGS) -I$$(Z8_ARM_DIR) -DPX_MISSION=$$(SC64_Z8_MISSION) $(2) $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z8-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Zerg-Z8-$(3).gba: build/starcraft-zerg-z8-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC Z8 REPLAY' SCZ8
endef
$(eval $(call Z8_ARM_ROM,replay,,Replay))
$(eval $(call Z8_ARM_ROM,replay-prefix,-DPX_REPLAY_PREFIX,Replay-Prefix))
$(eval $(call Z8_ARM_ROM,replay-resume,-DPX_REPLAY_RESUME,Replay-Resume))
.PHONY: sc64-z8-arm-replay
sc64-z8-arm-replay: build/StarCraft-Zerg-Z8-Replay.gba build/StarCraft-Zerg-Z8-Replay-Prefix.gba build/StarCraft-Zerg-Z8-Replay-Resume.gba

# Z9 candidate: the Z8 vector plus the authenticated Z9 program, rendered by
# `sc64_zerg_late_program_pack.py` from the private descriptor written by
# `sc64_zerg_late_pack.py pack --mission Z9`. ZERG_Z9_BANK must hold every Z9
# actor's art; it defaults to the Z8 bank (see the Z9 candidate audit).
ZERG_Z9_BANK ?= $(ZERG_Z8_BANK)
Z9_DESCRIPTOR ?= .local/build/z9-source/descriptor.json
Z9_PROGRAM_HEADER := build/generated/z9/sc64_z9_program_generated.h
$(Z9_PROGRAM_HEADER): $(Z9_DESCRIPTOR) tools/sc64_zerg_late_program_pack.py tools/sc64_zerg_late_pack.py tools/sc64_z8_program_pack.py | build
	$(PYTHON) tools/sc64_zerg_late_program_pack.py $< $@
ZERG_Z9_FEATURES = -DSC64_Z9_DATA_ENABLED -DSC64_Z9_RUNTIME -DSC64_Z9_CAMPAIGN_ENTRY -Ibuild/generated/z9
# Reuse the two Mode-4 page bands instead of retaining 12,480 HUD bytes in
# EWRAM. Z10 inherits this vector; set empty only for paired comparisons.
ZERG_Z9_HUD_PAGE_FLAGS ?= -DSC64_HUD_VRAM_BANDS
ZERG_Z9_CANONICAL_FLAGS = $(patsubst -I$(ZERG_Z8_BANK),-I$(ZERG_Z9_BANK),$(ZERG_Z8_CANONICAL_FLAGS)) $(ZERG_Z9_FEATURES) $(ZERG_Z9_HUD_PAGE_FLAGS)
ZERG_Z9_RELEASE_FLAGS = $(ZERG_Z9_CANONICAL_FLAGS) -DGBA_NO_DEBUG_INPUTS
ZERG_Z9_CANONICAL_INPUTS = $(filter-out $(ZERG_Z8_BANK)/sc64_race_sprites_generated.h,$(ZERG_Z8_CANONICAL_INPUTS)) $(ZERG_Z9_BANK)/sc64_race_sprites_generated.h $(Z9_PROGRAM_HEADER)
build/starcraft-zerg-z9.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_Z9_RELEASE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z9.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z9-Transport1-GBA.gba: build/starcraft-zerg-z9.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z9' SCZ9
.PHONY: verify-zerg-z9 zerg-z9
verify-zerg-z9: build/StarCraft-Zerg-Z9-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z9
zerg-z9: verify-zerg-z9
build/test-sc64-z9-game: tests/sc64_z9_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z9_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-zerg-z9
test-zerg-z9: build/test-sc64-z9-game
	./build/test-sc64-z9-game

# Z10 candidate: the Z9 vector plus the authenticated Z10 program, from
# `sc64_zerg_late_pack.py pack --mission Z10`. It carries Z9, so its test runs
# the Z9 fixture again under the Z10 flags.
ZERG_Z10_BANK ?= $(ZERG_Z9_BANK)
Z10_DESCRIPTOR ?= .local/build/z10-source/descriptor.json
Z10_PROGRAM_HEADER := build/generated/z10/sc64_z10_program_generated.h
$(Z10_PROGRAM_HEADER): $(Z10_DESCRIPTOR) tools/sc64_zerg_late_program_pack.py tools/sc64_zerg_late_pack.py tools/sc64_z8_program_pack.py | build
	$(PYTHON) tools/sc64_zerg_late_program_pack.py $< $@
# The ordinary Z10 renderer needs the same exact static key for lookup and
# capture. Compute it once per draw, as the other capable products do.
ZERG_Z10_DEFENSE_FLAGS ?= -DSC64_DEFENSE_SPATIAL_PROBE -DSC64_DEFENSE_PYLON_ROSTER
ZERG_Z10_FEATURES = -DSC64_Z10_DATA_ENABLED -DSC64_Z10_RUNTIME -DSC64_Z10_CAMPAIGN_ENTRY -DSC64_RENDER_KEY_ONCE -Ibuild/generated/z10 $(ZERG_Z10_DEFENSE_FLAGS) -DSC64_DEPOT_FIRST_NEAREST_PROBE
ZERG_Z10_CANONICAL_FLAGS = $(patsubst -I$(ZERG_Z9_BANK),-I$(ZERG_Z10_BANK),$(ZERG_Z9_CANONICAL_FLAGS)) $(ZERG_Z10_FEATURES)
# The packed frames belong to the qualified Z10 sprite bank.
Z10_BUILDING_WORDS ?= 1
ifneq ($(filter $(Z10_BUILDING_WORDS),0 1),$(Z10_BUILDING_WORDS))
$(error Z10_BUILDING_WORDS must be 0 or 1)
endif
SC64_BUILDING_WORDS_HEADER = build/generated/sc64_building_words_generated.h
Z10_BUILDING_WORDS_FLAGS = $(if $(filter 1,$(Z10_BUILDING_WORDS)),-DSC64_BUILDING_WORDS_PROBE)
Z10_BUILDING_WORDS_CONFIG := $(if $(shell test -f build/z10-building-words-config && printf '%s\n' '$(Z10_BUILDING_WORDS)' | cmp -s - build/z10-building-words-config && printf matched),,force-z10-building-words-rebuild)
ZERG_Z10_RELEASE_FLAGS = $(ZERG_Z10_CANONICAL_FLAGS) -DGBA_NO_DEBUG_INPUTS
ZERG_Z10_CANONICAL_INPUTS = $(filter-out $(ZERG_Z9_BANK)/sc64_race_sprites_generated.h,$(ZERG_Z9_CANONICAL_INPUTS)) $(ZERG_Z10_BANK)/sc64_race_sprites_generated.h $(Z10_PROGRAM_HEADER)
build/starcraft-zerg-z10.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.ld Makefile tests/sc64-zerg-late/Makefile build/z10-building-words-config $(Z10_BUILDING_WORDS_CONFIG) $(if $(Z10_BUILDING_WORDS_FLAGS),$(SC64_BUILDING_WORDS_HEADER)) | build
	$(CC) $(CFLAGS) $(ZERG_Z10_RELEASE_FLAGS) $(Z10_BUILDING_WORDS_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z10.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z10-Transport1-GBA.gba: build/starcraft-zerg-z10.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z10' SCZA
.PHONY: verify-zerg-z10 zerg-z10
verify-zerg-z10: build/StarCraft-Zerg-Z10-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z10
zerg-z10: verify-zerg-z10

build/test-sc64-z10-sight-packed-rows: tests/sc64_z7_source_sight_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) $(SC64_SIGHT_ROW_LUT_HEADER) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(filter-out -DSC64_Z7_VISIBILITY_PACKED_ROWS,$(ZERG_Z10_RELEASE_FLAGS)) -DSC64_Z7_VISIBILITY_PACKED_ROWS $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-z10-sight-packed-rows
test-sc64-z10-sight-packed-rows: build/test-sc64-z10-sight-packed-rows
	./build/test-sc64-z10-sight-packed-rows

.PHONY: force-z10-building-words-config force-z10-building-words-rebuild
force-z10-building-words-config:
force-z10-building-words-rebuild:
build/z10-building-words-config: force-z10-building-words-config | build
	@printf '%s\n' '$(Z10_BUILDING_WORDS)' > $@.tmp
	@cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@
$(SC64_BUILDING_WORDS_HEADER): $(ZERG_Z10_BANK)/sc64_race_sprites_generated.h tools/sc64_building_words.py tools/sc64_unit_facing.py | build
	mkdir -p $(dir $@)
	$(PYTHON) tools/sc64_building_words.py $< > $@.tmp
	mv $@.tmp $@
build/test-sc64-z10-building-words-pixels: tests/sc64_building_words_pixels.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) $(SC64_BUILDING_WORDS_HEADER) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(ZERG_Z10_RELEASE_FLAGS) -DSC64_BUILDING_WORDS_PROBE $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-z10-building-words-pixels
test-sc64-z10-building-words-pixels: build/test-sc64-z10-building-words-pixels
	./build/test-sc64-z10-building-words-pixels

ifeq ($(CANONICAL_WALK_WORDS),1)
build/test-sc64-z10-walk-words-pixels: tests/sc64_walk_words_pixels.c tests/sc64_unit_walk_frames_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(ZERG_Z10_RELEASE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-z10-walk-words-pixels
test-sc64-z10-walk-words-pixels: build/test-sc64-z10-walk-words-pixels
	./build/test-sc64-z10-walk-words-pixels
endif
build/test-sc64-z10-game: tests/sc64_z10_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z10-carried-z9: tests/sc64_z9_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-zerg-z10
test-zerg-z10: build/test-sc64-z10-game build/test-sc64-z10-carried-z9
	./build/test-sc64-z10-carried-z9
	./build/test-sc64-z10-game

# Exercise the shared Z8 game bridge with the carried Z10 configuration.
.PHONY: test-sc64-z10-command-census
test-sc64-z10-command-census: build/test-sc64-z10-command-census
	./build/test-sc64-z10-command-census
build/test-sc64-z10-command-census: tests/sc64_z8_command_census.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) -DSC64_Z8_DIRECT_COMMAND_CENSUS $< $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@

# Current Z10 differential for the optional defense spatial query.
build/test-sc64-z10-defense-spatial: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_RELEASE_FLAGS) -DSC64_DEFENSE_SPATIAL_PROBE $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-z10-defense-spatial
test-sc64-z10-defense-spatial: build/test-sc64-z10-defense-spatial
	./build/test-sc64-z10-defense-spatial

# Compare scalar power checks and the per-defense-pass Pylon roster using
# the production Z10 storage and targeting vector. The probe stays opt-in.
build/test-sc64-z10-defense-pylon-roster: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_RELEASE_FLAGS) -DSC64_DEFENSE_PYLON_ROSTER -DSC64_DEFENSE_PYLON_ROSTER_TEST_ONLY $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-z10-defense-pylon-roster
test-sc64-z10-defense-pylon-roster: build/test-sc64-z10-defense-pylon-roster
	./build/test-sc64-z10-defense-pylon-roster
build/test-sc64-z10-defense-combined: tests/sc64_zp_defense_targets.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_RELEASE_FLAGS) -DSC64_DEFENSE_SPATIAL_PROBE -DSC64_DEFENSE_PYLON_ROSTER $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-z10-defense-combined
test-sc64-z10-defense-combined: build/test-sc64-z10-defense-combined
	./build/test-sc64-z10-defense-combined

define Z10_DEPOT_FIRST_NEAREST_TEST
build/test-sc64-z10-depot-first-nearest-$(1): tests/sc64_depot_first_nearest.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $$(COMMON_HEADERS) tests/sc64-zerg-late/Makefile Makefile | build
	$$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $$(filter-out -DSC64_DEPOT_FIRST_NEAREST_PROBE,$$(ZERG_Z10_RELEASE_FLAGS)) $(2) $$< common/gba.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) -o $$@
endef
$(eval $(call Z10_DEPOT_FIRST_NEAREST_TEST,stock,))
$(eval $(call Z10_DEPOT_FIRST_NEAREST_TEST,probe,-DSC64_DEPOT_FIRST_NEAREST_PROBE))
.PHONY: test-sc64-z10-depot-first-nearest
test-sc64-z10-depot-first-nearest: build/test-sc64-z10-depot-first-nearest-stock build/test-sc64-z10-depot-first-nearest-probe
	./build/test-sc64-z10-depot-first-nearest-stock
	./build/test-sc64-z10-depot-first-nearest-probe


# Source-art coverage over both authenticated maps and their CreateUnit rows.
build/test-sc64-zerg-late-art: tests/sc64_zerg_late_art.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-zerg-late-art
test-sc64-zerg-late-art: build/test-sc64-zerg-late-art
	./build/test-sc64-zerg-late-art

# Z9 preparation must qualify its art without requiring Z10-only descriptors.
build/test-sc64-z9-art: tests/sc64_zerg_late_art.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-zerg-late/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z9-art
test-sc64-z9-art: build/test-sc64-z9-art
	./build/test-sc64-z9-art
test-sc64-zerg-z9: test-sc64-z9-art

# Z9 and Z10 ordinary-input qualification, as Z8's above.
# $(1) lower-case id, $(2) upper-case id, $(3) mission, $(4) ROM code.
define ZERG_LATE_ARM_ROM
build/starcraft-zerg-$(1)-$(5).elf: $$($(2)_REPLAY_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -I$$($(2)_ARM_DIR) -DPX_MISSION=$(3) -DPX_DIRECT_LOG -DPX_CARGO_DIGEST $(6) $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(LDFLAGS) -Wl,-Map,build/starcraft-zerg-$(1)-$(5).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Zerg-$(2)-$(7).gba: build/starcraft-zerg-$(1)-$(5).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ '$(if $(filter Z10,$(2)),SC Z10 RPLY,SC $(2) REPLAY)' $(4)
endef
define ZERG_LATE_QUALIFICATION
$(2)_ARM_DIR := build/$(1)-arm
$(2)_ARM_TRACE := build/$(1)-arm/$(1)-controller.jsonl
$(2)_TOOL = $$(PYTHON) tools/sc64_p8_arm_replay.py --mission $(3) --prefix PX
$(2)_REPLAY_DEPS = tests/sc64_protoss_arm_replay.c build/$(1)-arm/sc64_px_tape_generated.h $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(ZERG_$(2)_CANONICAL_INPUTS) common/gba.c $$(COMMON_HEADERS) tests/sc64-zerg-late/Makefile
build/test-sc64-$(1)-campaign-run: tests/sc64_$(1)_campaign_run.c tests/sc64_zerg_late_controller.h $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(ZERG_$(2)_CANONICAL_INPUTS) common/gba.c $$(COMMON_HEADERS) tests/sc64-zerg-late/Makefile | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) $$< -o $$@
build/$(1)-arm/$(1)-controller.jsonl: build/test-sc64-$(1)-campaign-run
	mkdir -p build/$(1)-arm
	./build/test-sc64-$(1)-campaign-run $$@
.PHONY: test-sc64-$(1)-controller
test-sc64-$(1)-controller: build/$(1)-arm/$(1)-controller.jsonl
build/$(1)-arm/sc64_px_tape_generated.h: build/$(1)-arm/$(1)-controller.jsonl tools/sc64_p8_arm_replay.py
	$$($(2)_TOOL) tape $$< $$@
build/test-sc64-$(1)-replay: $$($(2)_REPLAY_DEPS) | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -Ibuild/$(1)-arm -DPX_MISSION=$(3) -DPX_CARGO_DIGEST $$< -o $$@
build/test-sc64-$(1)-replay-prefix: $$($(2)_REPLAY_DEPS) | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -Ibuild/$(1)-arm -DPX_MISSION=$(3) -DPX_CARGO_DIGEST -DPX_REPLAY_PREFIX $$< -o $$@
build/test-sc64-$(1)-replay-resume: $$($(2)_REPLAY_DEPS) | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -Ibuild/$(1)-arm -DPX_MISSION=$(3) -DPX_CARGO_DIGEST -DPX_REPLAY_RESUME $$< -o $$@
.PHONY: test-sc64-$(1)-replay
test-sc64-$(1)-replay: build/test-sc64-$(1)-replay build/test-sc64-$(1)-replay-prefix build/test-sc64-$(1)-replay-resume tools/sc64_p8_arm_replay.py
	./build/test-sc64-$(1)-replay > build/$(1)-arm/host-full.txt
	./build/test-sc64-$(1)-replay-prefix build/$(1)-arm/host-prefix.sav > build/$(1)-arm/host-prefix.txt || true
	./build/test-sc64-$(1)-replay-resume build/$(1)-arm/host-prefix.sav > build/$(1)-arm/host-resume.txt
	$$($(2)_TOOL) compare --from-tick $$$$(sed -n 's/^#define PX_TAPE_RESUME_TICK \([0-9]*\)u$$$$/\1/p' build/$(1)-arm/sc64_px_tape_generated.h) build/$(1)-arm/host-full.txt build/$(1)-arm/host-resume.txt
.PHONY: test-sc64-zerg-$(1)
test-sc64-zerg-$(1): test-zerg-$(1) test-sc64-$(1)-controller test-sc64-$(1)-replay
$$(eval $$(call ZERG_LATE_ARM_ROM,$(1),$(2),$(3),$(4),replay,,Replay))
$$(eval $$(call ZERG_LATE_ARM_ROM,$(1),$(2),$(3),$(4),replay-prefix,-DPX_REPLAY_PREFIX,Replay-Prefix))
$$(eval $$(call ZERG_LATE_ARM_ROM,$(1),$(2),$(3),$(4),replay-resume,-DPX_REPLAY_RESUME,Replay-Resume))
.PHONY: sc64-$(1)-arm-replay
sc64-$(1)-arm-replay: build/StarCraft-Zerg-$(2)-Replay.gba build/StarCraft-Zerg-$(2)-Replay-Prefix.gba build/StarCraft-Zerg-$(2)-Replay-Resume.gba
endef
$(eval $(call ZERG_LATE_QUALIFICATION,z9,Z9,20,SCZ9))
$(eval $(call ZERG_LATE_QUALIFICATION,z10,Z10,21,SCZA))

# Z9 carried-profile normal-menu startup and save matrix.
build/test-sc64-z9-carried-profile: tests/sc64_promoted_race_profile.c tests/sc64_save_candidate_fixture.h $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z9-carried-profile
test-sc64-z9-carried-profile: build/test-sc64-z9-carried-profile
	./build/test-sc64-z9-carried-profile

# The ten-mission cartridge must preserve campaign and custom menus and saves.
build/test-sc64-z10-carried-profile: tests/sc64_promoted_race_profile.c tests/sc64_save_candidate_fixture.h $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-sc64-z10-carried-profile
test-sc64-z10-carried-profile: build/test-sc64-z10-carried-profile
	./build/test-sc64-z10-carried-profile
test-zerg-z10: test-sc64-z10-carried-profile

# Ordinary-input carried controllers must use the full cartridge vector,
# including the extra save and campaign code in Z10.
define ZERG_CARRIED_CONTROLLER
build/test-sc64-$(1)-carried-$(3): tests/$(4).c $(6) $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(ZERG_$(2)_CANONICAL_INPUTS) common/gba.c $$(COMMON_HEADERS) Makefile tests/sc64-zerg-late/Makefile | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) $$< $(5) -o $$@
endef
define ZERG_CARRIED_CONTROLLERS
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z1z2,race_campaign_run,common/gba.c))
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z3,sc64_z3_campaign_run,common/gba.c,tests/sc64_route_purity.h))
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z4,sc64_z4_campaign_run,common/gba.c,tests/sc64_route_purity.h))
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z5,sc64_z5_campaign_run,common/gba.c,tests/sc64_route_purity.h))
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z6,sc64_z6_campaign_run,common/gba.c,tests/sc64_route_purity.h))
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z7,sc64_z7_campaign_run,common/gba.c,tests/sc64_route_purity.h))
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z8,sc64_z8_campaign_run,))
$$(eval $$(call ZERG_CARRIED_CONTROLLER,$(1),$(2),z1,race_z1_controls,common/gba.c))
endef
$(eval $(call ZERG_CARRIED_CONTROLLERS,z9,Z9))
$(eval $(call ZERG_CARRIED_CONTROLLERS,z10,Z10))

# Z2 normal cartridge trace: reveal briefing pages and acknowledge authored
# messages with physical key pairs; the existing carried suite is unchanged.
build/test-sc64-z10-z2-normal-home: tests/race_campaign_run.c tests/sc64_controller_authored_pages.h tests/sc64_route_purity.h $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) -DSC64_Z2_NORMAL_HOME -DSC64_TEXT_BOX_HOST_MODAL $< common/gba.c -o $@

$(eval $(call ZERG_CARRIED_CONTROLLER,z10,Z10,z9-run,sc64_z9_campaign_run,))

# Z8 carried replay, compiled with the actual Z9 vector and strict cargo digest.
Z9_Z8_REPLAY_DEPS = tests/sc64_protoss_arm_replay.c build/z8-carried/sc64_px_tape_generated.h $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile
# The tape is generated from the retained input-only host controller trace.
define Z9_Z8_CARRIED_REPLAY
build/z9-z8-host-$(1): $$(Z9_Z8_REPLAY_DEPS) | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_Z9_CANONICAL_FLAGS) -Ibuild/z8-carried -DPX_MISSION=19 -DPX_CARGO_DIGEST $(2) $$< -o $$@
build/z9-z8-arm-$(1).elf: $$(Z9_Z8_REPLAY_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(ZERG_Z9_CANONICAL_FLAGS) -Ibuild/z8-carried -DPX_MISSION=19 -DPX_CARGO_DIGEST $(2) $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(LDFLAGS) -Wl,-Map,build/z9-z8-arm-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/Z9-Z8-$(1).gba: build/z9-z8-arm-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC Z9 Z8' SCZ9
endef
$(eval $(call Z9_Z8_CARRIED_REPLAY,full,))
$(eval $(call Z9_Z8_CARRIED_REPLAY,prefix,-DPX_REPLAY_PREFIX))
$(eval $(call Z9_Z8_CARRIED_REPLAY,resume,-DPX_REPLAY_RESUME))

# Z7 carried replay: Z9 engine flags and a separate strict cargo digest line.
Z9_Z7_REPLAY_DEPS = tests/sc64_z7_arm_replay.c build/z7-carried/sc64_z7_tape_generated.h $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile
Z9_Z7_CUT ?= 0
define Z9_Z7_CARRIED_REPLAY
build/z9-z7-host-$(1): $$(Z9_Z7_REPLAY_DEPS) | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_Z9_CANONICAL_FLAGS) -Ibuild/z7-carried -DZ7_CARGO_DIGEST $(2) $$< -o $$@
build/z9-z7-arm-$(1).elf: $$(Z9_Z7_REPLAY_DEPS) $$(Z7_ARM_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(ZERG_Z9_CANONICAL_FLAGS) -Ibuild/z7-carried -DZ7_CARGO_DIGEST $(2) $$(Z7_ARM_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(LDFLAGS) -Wl,-Map,build/z9-z7-arm-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/Z9-Z7-$(1).gba: build/z9-z7-arm-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC Z9 Z7' SCZ9
endef
$(eval $(call Z9_Z7_CARRIED_REPLAY,full,-DZ7_ARM_MODE=0))
$(eval $(call Z9_Z7_CARRIED_REPLAY,prefix,-DZ7_ARM_MODE=1 -DZ7_ARM_SAVE_MIN_TICK=40000u))
$(eval $(call Z9_Z7_CARRIED_REPLAY,resume,-DZ7_ARM_MODE=2 -DZ7_ARM_CUT_TICK=$(Z9_Z7_CUT)u))
# The carried Z10 runtime can fill its live unit bank before the historical
# Z7 policy starts buying air support. Start that controller's paid air plan
# earlier; the carried Z9 and standalone Z7 policies stay independent.
ZERG_Z9_CARRIED_Z7_POLICY = $(ZERG_Z7_CONTROLLER_POLICY)
ZERG_Z10_CARRIED_Z7_POLICY ?= $(ZERG_Z7_CONTROLLER_POLICY) --force-late-air-at-frame 85000
# Controller 0 of the generic early-race runner is not the established Z1 route.
define ZERG_CARRIED_MISSIONS
.PHONY: test-sc64-$(1)-carried-missions
test-sc64-$(1)-carried-missions: $$(addprefix build/test-sc64-$(1)-carried-,z1 z1z2 z3 z4 z5 z6 z7 z8)
	mkdir -p .local/sc64/$(1)-carried
	./build/test-sc64-$(1)-carried-z1 --trace .local/sc64/$(1)-carried/z1.json
	./build/test-sc64-$(1)-carried-z1z2 --controller 1 --trace .local/sc64/$(1)-carried/z2.json
	./build/test-sc64-$(1)-carried-z3 --policy hybrid --trace .local/sc64/$(1)-carried/z3.json
	./build/test-sc64-$(1)-carried-z4 --limit 150000 --trace .local/sc64/$(1)-carried/z4.json
	./build/test-sc64-$(1)-carried-z5 --limit 60000 --trace .local/sc64/$(1)-carried/z5-diagnostic.json --attempt .local/sc64/$(1)-carried/z5-attempt.json --verified .local/sc64/$(1)-carried/z5.json
	./build/test-sc64-$(1)-carried-z6 --trace .local/sc64/$(1)-carried/z6.json
	./build/test-sc64-$(1)-carried-z7 $$(ZERG_$(2)_CARRIED_Z7_POLICY) --limit $$(ZERG_Z7_CONTROLLER_LIMIT) --trace .local/sc64/$(1)-carried/z7.json
	./build/test-sc64-$(1)-carried-z8 .local/sc64/$(1)-carried/z8.jsonl
endef
$(eval $(call ZERG_CARRIED_MISSIONS,z9,Z9))
$(eval $(call ZERG_CARRIED_MISSIONS,z10,Z10))
.PHONY: test-sc64-z10-carried-z9-controller
test-sc64-z10-carried-z9-controller: build/test-sc64-z10-carried-z9-run
	mkdir -p .local/sc64/z10-carried
	./$< .local/sc64/z10-carried/z9.jsonl

# Full ordinary-input skirmish matches under each late Zerg product vector.
define ZERG_LATE_SKIRMISH_MATCHES
build/test-sc64-skirmish-match-zerg-$(1): tests/sc64_skirmish_campaign_run.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$(ZERG_$(2)_CANONICAL_INPUTS) $$(RACE_CAMPAIGN_INPUTS) common/gba.c common/gba.h Makefile tests/sc64-zerg-late/Makefile | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) $$< common/gba.c -o $$@
.PHONY: test-sc64-$(1)-canonical-skirmish
test-sc64-$(1)-canonical-skirmish: build/test-sc64-skirmish-match-zerg-$(1)
	$$(PYTHON) tools/sc64_skirmish_controller_matrix.py --controller $$< --output .local/sc64/skirmish/zerg-$(1)-matches
endef
$(eval $(call ZERG_LATE_SKIRMISH_MATCHES,z9,Z9))
$(eval $(call ZERG_LATE_SKIRMISH_MATCHES,z10,Z10))
test-sc64-zerg-z9: test-sc64-z9-carried-profile test-sc64-z9-carried-missions test-sc64-z9-canonical-skirmish
test-sc64-zerg-z10: test-sc64-z10-carried-missions test-sc64-z10-carried-z9-controller test-sc64-z10-canonical-skirmish

# Optional prepared-input path-grid parity: the Z5 fixture also exercises
# authored door actions and save/load. Extra include paths can select a
# separately verified cover bank without modifying the prepared inputs.
SC64_BULK_GRID_TEST_FLAGS ?=
Z9_BULK_GRID_DEPS = $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) tests/sc64-zerg-late/Makefile
build/test-sc64-z9-bulk-grid: tests/sc64_terrain_grid_pack_game.c $(Z9_BULK_GRID_DEPS) | build
	$(CLANG) $(Z8_HOST_FLAGS) -Wno-unused-parameter $(SC64_BULK_GRID_TEST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_BULK_TERRAIN_GRID -DSC64_BULK_TERRAIN_GRID_TEST $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-z9-bulk-z5: tests/sc64_z5_bulk_grid.c tests/sc64_z5_runtime.c $(Z9_BULK_GRID_DEPS) | build
	$(CLANG) $(Z8_HOST_FLAGS) -Wno-unused-parameter $(SC64_BULK_GRID_TEST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_BULK_TERRAIN_GRID -DSC64_BULK_TERRAIN_GRID_TEST $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-z9-bulk-grid
test-sc64-z9-bulk-grid: build/test-sc64-z9-bulk-grid build/test-sc64-z9-bulk-z5
	./build/test-sc64-z9-bulk-grid
	./build/test-sc64-z9-bulk-z5
