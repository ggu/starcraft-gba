# SPDX-License-Identifier: GPL-3.0-or-later
.PHONY: test-sc64-research-q8-live
Q8_EXTRA_FLAGS ?=
Q8_FACT_INCLUDE ?=
Q8_SCARAB_FLAGS = -DSC64_SCARAB_RUNTIME -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE
.PHONY: prepare-sc64-research-q8-scarab
prepare-sc64-research-q8-scarab:
	@mkdir -p build/generated/q8-scarab
	$(CLANG) $(P8_HOST_FLAGS) $(filter-out -DSC64_FACT_TABLES -Ibuild/generated/p10-fact-v2,$(PROTOSS_P10_FLAGS)) $(Q8_SCARAB_FLAGS) tools/sc64_fact_tables_gen.c -o build/generated/q8-scarab/generator
	./build/generated/q8-scarab/generator > build/generated/q8-scarab/sc64_fact_tables_generated.h

test-sc64-research-q8-live:
	@mkdir -p build/research-q8
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(Q8_FACT_INCLUDE) $(PROTOSS_P10_FLAGS) $(Q8_EXTRA_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT tests/sc64_research_q8_live_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8/live-p10
	./build/research-q8/live-p10

# Operator-supplied qualified product assets. No save format is enabled.
Q8_LIFECYCLE_FLAGS = -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT
Q8_LIFECYCLE_SOURCES = tests/sc64_research_q8_lifecycle_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
.PHONY: test-sc64-research-q8-lifecycle-p10 test-sc64-research-q8-lifecycle-z9 test-sc64-research-q8-lifecycle-terran
test-sc64-research-q8-lifecycle-p10:
	@mkdir -p build/research-q8
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) $(Q8_LIFECYCLE_FLAGS) $(Q8_LIFECYCLE_SOURCES) -o build/research-q8/lifecycle-p10
	./build/research-q8/lifecycle-p10
test-sc64-research-q8-lifecycle-z9:
	@mkdir -p build/research-q8
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(ZERG_Z9_CANONICAL_FLAGS) $(Q8_LIFECYCLE_FLAGS) $(Q8_LIFECYCLE_SOURCES) -o build/research-q8/lifecycle-z9
	./build/research-q8/lifecycle-z9
test-sc64-research-q8-lifecycle-terran:
	@mkdir -p build/research-q8
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(TERRAN_ROM_FLAGS) $(Q8_LIFECYCLE_FLAGS) $(Q8_LIFECYCLE_SOURCES) -o build/research-q8/lifecycle-terran
	./build/research-q8/lifecycle-terran
