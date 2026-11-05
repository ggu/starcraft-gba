include tests/sc64-guardian-egg-journal/Makefile
CUSTOM_HOST = -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_SHARED_EGG_CUSTOM_SERVICE
CUSTOM_EXTRA = tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
.PHONY: custom-egg-terran custom-egg-z7 custom-egg-z9 custom-egg-p10 custom-egg-showcase
custom-egg-terran:
	$(CLANG) $(CUSTOM_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-terran
	$(COMBINED_BIN_DIR)/custom-egg-terran
custom-egg-z7:
	$(CLANG) $(CUSTOM_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-z7
	$(COMBINED_BIN_DIR)/custom-egg-z7
custom-egg-z9:
	$(CLANG) $(CUSTOM_HOST) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-z9
	$(COMBINED_BIN_DIR)/custom-egg-z9
custom-egg-p10:
	$(CLANG) $(CUSTOM_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(CUSTOM_FACT_DIR),$(PROTOSS_P10_FLAGS)) tests/sc64_shared_egg_ordinary_input.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-p10
	$(COMBINED_BIN_DIR)/custom-egg-p10
custom-egg-showcase:
	$(CLANG) $(CUSTOM_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-showcase
	$(COMBINED_BIN_DIR)/custom-egg-showcase

.PHONY: custom-egg-journal-terran custom-egg-journal-z7 custom-egg-journal-z9 custom-egg-journal-p10 custom-egg-journal-showcase
custom-egg-journal-terran:
	$(CLANG) $(CUSTOM_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_custom_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-journal-terran
	$(COMBINED_BIN_DIR)/custom-egg-journal-terran
custom-egg-journal-z7:
	$(CLANG) $(CUSTOM_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_custom_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-journal-z7
	$(COMBINED_BIN_DIR)/custom-egg-journal-z7
custom-egg-journal-z9:
	$(CLANG) $(CUSTOM_HOST) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_egg_custom_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-journal-z9
	$(COMBINED_BIN_DIR)/custom-egg-journal-z9
custom-egg-journal-p10:
	$(CLANG) $(CUSTOM_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(CUSTOM_FACT_DIR),$(PROTOSS_P10_FLAGS)) tests/sc64_shared_egg_custom_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-journal-p10
	$(COMBINED_BIN_DIR)/custom-egg-journal-p10
custom-egg-journal-showcase:
	$(CLANG) $(CUSTOM_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_custom_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-journal-showcase
	$(COMBINED_BIN_DIR)/custom-egg-journal-showcase

CUSTOM_FACT_DIR ?= .local/custom-egg-p10-facts
.PHONY: custom-egg-facts
custom-egg-facts:
	mkdir -p $(CUSTOM_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -DSC64_SHARED_EGG_WIRE_HOST_STAGE $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o $(CUSTOM_FACT_DIR)/generator
	$(CUSTOM_FACT_DIR)/generator > $(CUSTOM_FACT_DIR)/sc64_fact_tables_generated.h
