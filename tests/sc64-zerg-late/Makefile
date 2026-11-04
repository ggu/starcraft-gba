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
ZERG_Z9_CANONICAL_FLAGS = $(patsubst -I$(ZERG_Z8_BANK),-I$(ZERG_Z9_BANK),$(ZERG_Z8_CANONICAL_FLAGS)) $(ZERG_Z9_FEATURES)
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
ZERG_Z10_FEATURES = -DSC64_Z10_DATA_ENABLED -DSC64_Z10_RUNTIME -DSC64_Z10_CAMPAIGN_ENTRY -Ibuild/generated/z10
ZERG_Z10_CANONICAL_FLAGS = $(patsubst -I$(ZERG_Z9_BANK),-I$(ZERG_Z10_BANK),$(ZERG_Z9_CANONICAL_FLAGS)) $(ZERG_Z10_FEATURES)
ZERG_Z10_RELEASE_FLAGS = $(ZERG_Z10_CANONICAL_FLAGS) -DGBA_NO_DEBUG_INPUTS
ZERG_Z10_CANONICAL_INPUTS = $(filter-out $(ZERG_Z9_BANK)/sc64_race_sprites_generated.h,$(ZERG_Z9_CANONICAL_INPUTS)) $(ZERG_Z10_BANK)/sc64_race_sprites_generated.h $(Z10_PROGRAM_HEADER)
build/starcraft-zerg-z10.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.ld Makefile | build
	$(CC) $(CFLAGS) $(ZERG_Z10_RELEASE_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-z10.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z10-Transport1-GBA.gba: build/starcraft-zerg-z10.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG Z10' SCZA
.PHONY: verify-zerg-z10 zerg-z10
verify-zerg-z10: build/StarCraft-Zerg-Z10-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --zerg-z10
zerg-z10: verify-zerg-z10
build/test-sc64-z10-game: tests/sc64_z10_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z10-carried-z9: tests/sc64_z9_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(ZERG_Z10_CANONICAL_FLAGS) $< common/gba.c -o $@
.PHONY: test-zerg-z10
test-zerg-z10: build/test-sc64-z10-game build/test-sc64-z10-carried-z9
	./build/test-sc64-z10-carried-z9
	./build/test-sc64-z10-game

# Z9 and Z10 ordinary-input qualification, as Z8's above.
# $(1) lower-case id, $(2) upper-case id, $(3) mission, $(4) ROM code.
define ZERG_LATE_ARM_ROM
build/starcraft-zerg-$(1)-$(5).elf: $$($(2)_REPLAY_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -I$$($(2)_ARM_DIR) -DPX_MISSION=$(3) -DPX_DIRECT_LOG $(6) $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(LDFLAGS) -Wl,-Map,build/starcraft-zerg-$(1)-$(5).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Zerg-$(2)-$(7).gba: build/starcraft-zerg-$(1)-$(5).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC $(2) REPLAY' $(4)
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
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -Ibuild/$(1)-arm -DPX_MISSION=$(3) $$< -o $$@
build/test-sc64-$(1)-replay-prefix: $$($(2)_REPLAY_DEPS) | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -Ibuild/$(1)-arm -DPX_MISSION=$(3) -DPX_REPLAY_PREFIX $$< -o $$@
build/test-sc64-$(1)-replay-resume: $$($(2)_REPLAY_DEPS) | build
	$$(CLANG) $$(Z8_HOST_FLAGS) $$(ZERG_$(2)_CANONICAL_FLAGS) -Ibuild/$(1)-arm -DPX_MISSION=$(3) -DPX_REPLAY_RESUME $$< -o $$@
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
