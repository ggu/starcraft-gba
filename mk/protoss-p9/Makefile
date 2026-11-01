# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit P9 candidate: the P1-P8 candidate plus Shadow Hunters as a ninth
# entry, with SCP9, generation-45 saves and fact tables for its roster.
P9_FACT_V2_DIR := build/generated/p9-fact-v2
P9_FACT_V2_HEADER := $(P9_FACT_V2_DIR)/sc64_fact_tables_generated.h
P9_SCENARIO_HEADER := .local/sc64/zp-stage/generated/scenarios/sc64_p9_scenario_generated.h
P9_CAMPAIGN_DEFINES := -DSC64_P9_DATA_ENABLED -DSC64_P9_RUNTIME -DSC64_P9_CAMPAIGN_ENTRY
PROTOSS_P9_FLAGS = $(patsubst -I$(P8_FACT_V2_DIR),-I$(P9_FACT_V2_DIR),$(PROTOSS_P8_FLAGS)) $(P9_CAMPAIGN_DEFINES)
PROTOSS_P9_INPUTS = $(filter-out $(P8_FACT_V2_HEADER),$(PROTOSS_P8_INPUTS)) $(P9_FACT_V2_HEADER) $(P9_SCENARIO_HEADER)
$(P9_FACT_V2_HEADER): $(P8_AVAILABILITY_HEADER) tools/sc64_fact_tables_gen.c $(wildcard starcraft/*.h) $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile | build
	mkdir -p $(P9_FACT_V2_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P9_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o build/sc64-p9-fact-v2-gen
	./build/sc64-p9-fact-v2-gen > $@.tmp
	mv $@.tmp $@
build/starcraft-protoss-p9.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P9_INPUTS) common/gba.ld Makefile mk/protoss-p9/Makefile | build
	$(CC) $(CFLAGS) $(PROTOSS_P9_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p9.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P9-Transport1-GBA.gba: build/starcraft-protoss-p9.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS9' SCP9
.PHONY: protoss-p9 verify-protoss-p9
verify-protoss-p9: build/StarCraft-Protoss-P9-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p9
protoss-p9: verify-protoss-p9

# P9 ordinary-input qualification, built like the P8 one above.
P9_ARM_DIR := build/p9-arm
P9_ARM_TRACE := $(P9_ARM_DIR)/p9-controller.jsonl
P9_REPLAY_TOOL := $(PYTHON) tools/sc64_p8_arm_replay.py --mission 28 --prefix P9
build/test-sc64-p9-carried-profile: tests/sc64_promoted_race_profile.c tests/sc64_save_candidate_fixture.h $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P9_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P9_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-p9-carried-profile
test-sc64-p9-carried-profile: build/test-sc64-p9-carried-profile
	./build/test-sc64-p9-carried-profile

build/test-sc64-p9-campaign-run: tests/sc64_p9_campaign_run.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P9_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P9_FLAGS) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
$(P9_ARM_TRACE): build/test-sc64-p9-campaign-run
	mkdir -p $(P9_ARM_DIR)
	./build/test-sc64-p9-campaign-run $@
.PHONY: test-sc64-p9-controller
test-sc64-p9-controller: $(P9_ARM_TRACE)
$(P9_ARM_DIR)/sc64_p9_tape_generated.h: $(P9_ARM_TRACE) tools/sc64_p8_arm_replay.py mk/protoss-p9/Makefile
	$(P9_REPLAY_TOOL) tape $< $@
P9_REPLAY_DEPS = tests/sc64_p9_arm_replay.c $(P9_ARM_DIR)/sc64_p9_tape_generated.h $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P9_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile
build/test-sc64-p9-replay: $(P9_REPLAY_DEPS) | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P9_FLAGS) -I$(P9_ARM_DIR) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-p9-replay-prefix: $(P9_REPLAY_DEPS) | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P9_FLAGS) -I$(P9_ARM_DIR) -DP9_REPLAY_PREFIX $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-p9-replay-resume: $(P9_REPLAY_DEPS) | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P9_FLAGS) -I$(P9_ARM_DIR) -DP9_REPLAY_RESUME $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-p9-replay test-sc64-protoss-p9
test-sc64-p9-replay: build/test-sc64-p9-replay build/test-sc64-p9-replay-prefix build/test-sc64-p9-replay-resume tools/sc64_p8_arm_replay.py
	./build/test-sc64-p9-replay > $(P9_ARM_DIR)/host-full.txt
	rm -f $(P9_ARM_DIR)/host-prefix.sav
	./build/test-sc64-p9-replay-prefix $(P9_ARM_DIR)/host-prefix.sav > $(P9_ARM_DIR)/host-prefix.txt || true
	./build/test-sc64-p9-replay-resume $(P9_ARM_DIR)/host-prefix.sav > $(P9_ARM_DIR)/host-resume.txt
	$(P9_REPLAY_TOOL) compare --from-tick $$(sed -n 's/^#define P9_TAPE_RESUME_TICK \([0-9]*\)u$$/\1/p' $(P9_ARM_DIR)/sc64_p9_tape_generated.h) $(P9_ARM_DIR)/host-full.txt $(P9_ARM_DIR)/host-resume.txt
test-sc64-protoss-p9: test-sc64-p9-controller test-sc64-p9-replay
define P9_ARM_ROM
build/starcraft-protoss-p9-$(1).elf: $$(P9_REPLAY_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(PROTOSS_P9_FLAGS) -I$$(P9_ARM_DIR) $(2) $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p9-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Protoss-P9-$(3).gba: build/starcraft-protoss-p9-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC P9 REPLAY' SCP9
endef
$(eval $(call P9_ARM_ROM,replay,,Replay))
$(eval $(call P9_ARM_ROM,replay-prefix,-DP9_REPLAY_PREFIX,Replay-Prefix))
$(eval $(call P9_ARM_ROM,replay-resume,-DP9_REPLAY_RESUME,Replay-Resume))
.PHONY: sc64-p9-arm-replay
sc64-p9-arm-replay: build/StarCraft-Protoss-P9-Replay.gba build/StarCraft-Protoss-P9-Replay-Prefix.gba build/StarCraft-Protoss-P9-Replay-Resume.gba
