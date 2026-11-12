include Makefile
COMBINED_BIN_DIR ?= /tmp
COMBINED_FACT_DIR ?= .local/guardian-egg-p10-facts
COMBINED_HOST = -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_GUARDIAN_COCOON_STAGE -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE
COMBINED_EXTRA = tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
.PHONY: combined-stage-facts
combined-stage-facts:
	mkdir -p $(COMBINED_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -DSC64_GUARDIAN_COCOON_STAGE -DSC64_SHARED_EGG_WIRE_HOST_STAGE $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o $(COMBINED_FACT_DIR)/generator
	$(COMBINED_FACT_DIR)/generator > $(COMBINED_FACT_DIR)/sc64_fact_tables_generated.h
.PHONY: combined-journal-terran combined-journal-zerg combined-journal-protoss combined-journal-showcase
combined-journal-terran:
	$(CLANG) $(COMBINED_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_egg_combined_journal.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/combined-journal-terran
	$(COMBINED_BIN_DIR)/combined-journal-terran
combined-journal-zerg: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(COMBINED_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_egg_combined_journal.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/combined-journal-zerg
	$(COMBINED_BIN_DIR)/combined-journal-zerg
combined-journal-protoss:
	$(CLANG) $(COMBINED_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(COMBINED_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS)) tests/sc64_guardian_egg_combined_journal.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/combined-journal-protoss
	$(COMBINED_BIN_DIR)/combined-journal-protoss
combined-journal-showcase:
	$(CLANG) $(COMBINED_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_egg_combined_journal.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/combined-journal-showcase
	$(COMBINED_BIN_DIR)/combined-journal-showcase
