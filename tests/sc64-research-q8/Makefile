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

.PHONY: test-sc64-research-q8-storm-p10 test-sc64-research-q8-storm-z9
test-sc64-research-q8-storm-p10:
	@mkdir -p build/research-q8
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) $(Q8_LIFECYCLE_FLAGS) -DQ8_STORM_EXPECTED_SCENES=2 tests/sc64_research_q8_storm_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8/storm-p10
	./build/research-q8/storm-p10
test-sc64-research-q8-storm-z9:
	@mkdir -p build/research-q8
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(ZERG_Z9_CANONICAL_FLAGS) $(Q8_LIFECYCLE_FLAGS) -DQ8_STORM_EXPECTED_SCENES=1 tests/sc64_research_q8_storm_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8/storm-z9
	./build/research-q8/storm-z9

# Explicit HOST-only actual-game Flash experiment. No normal ROM activation.
Q8_BASELINE_ROOT ?= .local/q8-flash-baseline
.PHONY: test-sc64-q8-flash-host-p10 check-sc64-q8-flash-baseline
check-sc64-q8-flash-baseline:
	python3 tests/sc64-research-q8/check_flash_baseline.py "$(Q8_BASELINE_ROOT)"
test-sc64-q8-flash-host-p10: check-sc64-q8-flash-baseline
	@mkdir -p build/research-q8-flash
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT -DSC64_Q8_FLASH_HOST_STAGE tests/sc64_q8_flash_game.c common/gba_flash1m.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8-flash/consumer
	$(CLANG) -I$(Q8_BASELINE_ROOT)/common -I$(Q8_BASELINE_ROOT)/starcraft $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) -DSC64_Q8_FLASH_BASELINE tests/sc64_q8_flash_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8-flash/old-producer
	./build/research-q8-flash/old-producer old-write build/research-q8-flash/old.sav
	./build/research-q8-flash/consumer unknown-hash build/research-q8-flash/old.sav
	./build/research-q8-flash/consumer unknown-length build/research-q8-flash/old.sav
	./build/research-q8-flash/consumer mixed-47 build/research-q8-flash/old.sav
	./build/research-q8-flash/consumer race-ownership build/research-q8-flash/old.sav
	./build/research-q8-flash/old-producer old-phase-write build/research-q8-flash/old-phase.sav
	./build/research-q8-flash/consumer read-old-phase build/research-q8-flash/old-phase.sav

	./build/research-q8-flash/consumer migrate build/research-q8-flash/old.sav
	./build/research-q8-flash/consumer read build/research-q8-flash/old.sav
	./build/research-q8-flash/consumer fallback build/research-q8-flash/old.sav
	./build/research-q8-flash/consumer write build/research-q8-flash/new.sav
	./build/research-q8-flash/consumer read build/research-q8-flash/new.sav
	./build/research-q8-flash/consumer reject-new build/research-q8-flash/new.sav

.PHONY: build-sc64-q8-flash-phase
build-sc64-q8-flash-phase: check-sc64-q8-flash-baseline
	@mkdir -p build/research-q8-flash-phase
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT -DSC64_Q8_FLASH_HOST_STAGE tests/sc64_q8_flash_phase_game.c common/gba_flash1m.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8-flash-phase/consumer
	$(CLANG) -I$(Q8_BASELINE_ROOT)/common -I$(Q8_BASELINE_ROOT)/starcraft $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) -DSC64_Q8_FLASH_BASELINE tests/sc64_q8_flash_phase_game.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8-flash-phase/old-producer

# Explicit HOST-only authored cross-race Flash writer/reader; no normal enable.
.PHONY: test-sc64-q8-flash-custom-races
test-sc64-q8-flash-custom-races:
	@mkdir -p build/research-q8-flash
	$(CLANG) $(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $(PROTOSS_P10_FLAGS) -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT -DSC64_Q8_FLASH_HOST_STAGE -DSC64_Q8_FLASH_ALL_CUSTOM_HOST tests/sc64_q8_flash_custom_races_game.c common/gba_flash1m.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o build/research-q8-flash/custom-races
	./build/research-q8-flash/custom-races write terran build/research-q8-flash/custom-terran.sav
	./build/research-q8-flash/custom-races read terran build/research-q8-flash/custom-terran.sav
	./build/research-q8-flash/custom-races write zerg build/research-q8-flash/custom-zerg.sav
	./build/research-q8-flash/custom-races read zerg build/research-q8-flash/custom-zerg.sav

# The supervising caller holds the shared native lock and freezes inputs.
.PHONY: test-sc64-q8-flash-custom-matrix
Q8_CUSTOM_MATRIX_OUT ?= build/research-q8-flash/custom-matrix
test-sc64-q8-flash-custom-matrix: test-sc64-q8-flash-custom-races
	$(PYTHON) tools/sc64_q8_flash_custom_matrix.py --fixture build/research-q8-flash/custom-races --output $(Q8_CUSTOM_MATRIX_OUT)
