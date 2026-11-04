# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the P9 candidate rules; it is not
# a standalone makefile. Run these targets from the repository root.

# Explicit P10 candidate: the P1-P9 candidate plus Eye of the Storm as a tenth
# entry, with SCPA, generation-47 saves and fact tables for its roster.
P10_FACT_V2_DIR := build/generated/p10-fact-v2
P10_FACT_V2_HEADER := $(P10_FACT_V2_DIR)/sc64_fact_tables_generated.h
P10_SCENARIO_HEADER := .local/sc64/zp-stage/generated/scenarios/sc64_p10_scenario_generated.h
# Probes and SCVs serve two bases far apart, so cargo goes to the nearest depot.
P10_CAMPAIGN_DEFINES := -DSC64_P10_DATA_ENABLED -DSC64_P10_RUNTIME -DSC64_P10_CAMPAIGN_ENTRY -DSC64_NEAREST_RESOURCE_DEPOT
PROTOSS_P10_FLAGS = $(patsubst -I$(P9_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(PROTOSS_P9_FLAGS)) $(P10_CAMPAIGN_DEFINES)
PROTOSS_P10_INPUTS = $(filter-out $(P9_FACT_V2_HEADER),$(PROTOSS_P9_INPUTS)) $(P10_FACT_V2_HEADER) $(P10_SCENARIO_HEADER)
$(P10_FACT_V2_HEADER): $(P8_AVAILABILITY_HEADER) tools/sc64_fact_tables_gen.c $(wildcard starcraft/*.h) $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile | build
	mkdir -p $(P10_FACT_V2_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o build/sc64-p10-fact-v2-gen
	./build/sc64-p10-fact-v2-gen > $@.tmp
	mv $@.tmp $@
build/starcraft-protoss-p10.elf: $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.ld Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile | build
	$(CC) $(CFLAGS) $(PROTOSS_P10_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p10.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Protoss-P10-Transport1-GBA.gba: build/starcraft-protoss-p10.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC PROTOSS10' SCPA
.PHONY: protoss-p10 verify-protoss-p10
verify-protoss-p10: build/StarCraft-Protoss-P10-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p10
protoss-p10: verify-protoss-p10
# P10 is the default Protoss product; explicit protoss-p7 keeps its own identity.
.PHONY: verify-protoss-p7 test-protoss test-protoss-p7
verify-protoss-p7: build/StarCraft-Protoss-P7-Transport1-GBA.gba
	$(PYTHON) tools/verify_roms.py --protoss-p7
test-protoss: test-protoss-p7 test-sc64-protoss-p10

# P10 host checks: the P1-P10 carried-profile startup/save matrix, and the
# P10 mission run (start-up program, objectives and a pause-menu save that
# reloads in a fresh process and continues identically).
P10_HOST_DIR := build/p10-host
build/test-sc64-p10-carried-profile: tests/sc64_promoted_race_profile.c tests/sc64_save_candidate_fixture.h $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-p10-carried-profile
test-sc64-p10-carried-profile: build/test-sc64-p10-carried-profile
	./build/test-sc64-p10-carried-profile
build/test-sc64-p10-mission: tests/sc64_p10_mission.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-p10-mission test-sc64-protoss-p10
test-sc64-p10-mission: build/test-sc64-p10-mission
	mkdir -p $(P10_HOST_DIR)
	./build/test-sc64-p10-mission $(P10_HOST_DIR)
test-sc64-protoss-p10: test-sc64-p10-mission test-sc64-p10-carried-profile

# P10 ordinary-input qualification, built like the P8 and P9 ones.
P10_ARM_DIR := build/p10-arm
P10_ARM_TRACE := $(P10_ARM_DIR)/p10-controller.jsonl
P10_REPLAY_TOOL := $(PYTHON) tools/sc64_p8_arm_replay.py --mission 29 --prefix P10
build/test-sc64-p10-campaign-run: tests/sc64_p10_campaign_run.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
$(P10_ARM_TRACE): build/test-sc64-p10-campaign-run
	mkdir -p $(P10_ARM_DIR)
	./build/test-sc64-p10-campaign-run $@
.PHONY: test-sc64-p10-controller
test-sc64-p10-controller: $(P10_ARM_TRACE)
$(P10_ARM_DIR)/sc64_p10_tape_generated.h: $(P10_ARM_TRACE) tools/sc64_p8_arm_replay.py mk/protoss-p10/Makefile
	$(P10_REPLAY_TOOL) tape $< $@
P10_REPLAY_DEPS = tests/sc64_p10_arm_replay.c $(P10_ARM_DIR)/sc64_p10_tape_generated.h $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile
build/test-sc64-p10-replay: $(P10_REPLAY_DEPS) | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) -I$(P10_ARM_DIR) $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-p10-replay-prefix: $(P10_REPLAY_DEPS) | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) -I$(P10_ARM_DIR) -DP10_REPLAY_PREFIX $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-p10-replay-resume: $(P10_REPLAY_DEPS) | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) -I$(P10_ARM_DIR) -DP10_REPLAY_RESUME $< $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-p10-replay
test-sc64-p10-replay: build/test-sc64-p10-replay build/test-sc64-p10-replay-prefix build/test-sc64-p10-replay-resume tools/sc64_p8_arm_replay.py
	./build/test-sc64-p10-replay > $(P10_ARM_DIR)/host-full.txt
	rm -f $(P10_ARM_DIR)/host-prefix.sav
	./build/test-sc64-p10-replay-prefix $(P10_ARM_DIR)/host-prefix.sav > $(P10_ARM_DIR)/host-prefix.txt || true
	./build/test-sc64-p10-replay-resume $(P10_ARM_DIR)/host-prefix.sav > $(P10_ARM_DIR)/host-resume.txt
	$(P10_REPLAY_TOOL) compare --from-tick $$(sed -n 's/^#define P10_TAPE_RESUME_TICK \([0-9]*\)u$$/\1/p' $(P10_ARM_DIR)/sc64_p10_tape_generated.h) $(P10_ARM_DIR)/host-full.txt $(P10_ARM_DIR)/host-resume.txt
test-sc64-protoss-p10: test-sc64-p10-controller test-sc64-p10-replay
define P10_ARM_ROM
build/starcraft-protoss-p10-$(1).elf: $$(P10_REPLAY_DEPS) $$(P8_REPLAY_COMMON) common/gba.ld | build
	$$(CC) $$(CFLAGS) $$(PROTOSS_P10_FLAGS) -I$$(P10_ARM_DIR) $(2) $$(P8_REPLAY_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(LDFLAGS) -Wl,-Map,build/starcraft-protoss-p10-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Protoss-P10-$(3).gba: build/starcraft-protoss-p10-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC P10 RPLAY' SCPA
endef
$(eval $(call P10_ARM_ROM,replay,,Replay))
$(eval $(call P10_ARM_ROM,replay-prefix,-DP10_REPLAY_PREFIX,Replay-Prefix))
$(eval $(call P10_ARM_ROM,replay-resume,-DP10_REPLAY_RESUME,Replay-Resume))
.PHONY: sc64-p10-arm-replay
sc64-p10-arm-replay: build/StarCraft-Protoss-P10-Replay.gba build/StarCraft-Protoss-P10-Replay-Prefix.gba build/StarCraft-Protoss-P10-Replay-Resume.gba
