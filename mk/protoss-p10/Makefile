# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the P9 candidate rules; it is not
# a standalone makefile. Run these targets from the repository root.

# Explicit P10 candidate: the P1-P9 candidate plus Eye of the Storm as a tenth
# entry, with SCPA, generation-47 saves and fact tables for its roster.
P10_FACT_V2_DIR := build/generated/p10-fact-v2
P10_FACT_V2_HEADER := $(P10_FACT_V2_DIR)/sc64_fact_tables_generated.h
P10_SCENARIO_HEADER := .local/sc64/zp-stage/generated/scenarios/sc64_p10_scenario_generated.h
P10_CAMPAIGN_DEFINES := -DSC64_P10_DATA_ENABLED -DSC64_P10_RUNTIME -DSC64_P10_CAMPAIGN_ENTRY
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
