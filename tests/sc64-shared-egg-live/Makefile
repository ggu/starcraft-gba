include tests/sc64-guardian-egg-journal/Makefile
LIVE_HOST = $(COMBINED_HOST) -DSC64_SHARED_EGG_PRODUCER_HOST_STAGE
.PHONY: shared-egg-live-terran shared-egg-live-z7 shared-egg-live-z9 shared-egg-live-p10 shared-egg-live-showcase
shared-egg-live-terran:
	$(CLANG) $(LIVE_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_live_game.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-live-terran
	$(COMBINED_BIN_DIR)/shared-egg-live-terran
shared-egg-live-z7:
	$(CLANG) $(LIVE_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_live_game.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-live-z7
	$(COMBINED_BIN_DIR)/shared-egg-live-z7
shared-egg-live-z9:
	$(CLANG) $(LIVE_HOST) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_egg_live_game.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-live-z9
	$(COMBINED_BIN_DIR)/shared-egg-live-z9
shared-egg-live-p10:
	$(CLANG) $(LIVE_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(COMBINED_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS)) tests/sc64_shared_egg_live_game.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-live-p10
	$(COMBINED_BIN_DIR)/shared-egg-live-p10
shared-egg-live-showcase:
	$(CLANG) $(LIVE_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_live_game.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-live-showcase
	$(COMBINED_BIN_DIR)/shared-egg-live-showcase

.PHONY: shared-egg-input-terran shared-egg-input-z7 shared-egg-input-z9 shared-egg-input-p10 shared-egg-input-showcase
shared-egg-input-terran:
	$(CLANG) $(LIVE_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-input-terran
	$(COMBINED_BIN_DIR)/shared-egg-input-terran
shared-egg-input-z7:
	$(CLANG) $(LIVE_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-input-z7
	$(COMBINED_BIN_DIR)/shared-egg-input-z7
shared-egg-input-z9:
	$(CLANG) $(LIVE_HOST) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-input-z9
	$(COMBINED_BIN_DIR)/shared-egg-input-z9
shared-egg-input-p10:
	$(CLANG) $(LIVE_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(COMBINED_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS)) tests/sc64_shared_egg_ordinary_input.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-input-p10
	$(COMBINED_BIN_DIR)/shared-egg-input-p10
shared-egg-input-showcase:
	$(CLANG) $(LIVE_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_ordinary_input.c $(COMBINED_EXTRA) -o $(COMBINED_BIN_DIR)/shared-egg-input-showcase
	$(COMBINED_BIN_DIR)/shared-egg-input-showcase
