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

.PHONY: custom-egg-z7-legacy
custom-egg-z7-legacy:
	$(CLANG) $(CUSTOM_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_custom_z7_legacy.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-z7-legacy
	$(COMBINED_BIN_DIR)/custom-egg-z7-legacy

.PHONY: custom-egg-conflict-guards
custom-egg-conflict-guards:
	$(PYTHON) tests/check_sc64_shared_egg_custom_guards.py $(CLANG) $(CUSTOM_HOST) $(ZERG_Z9_CANONICAL_FLAGS)

# PRE_CARGO_OLD_ROOT must name a clean 17dc9b3e checkout with matching assets.
# Constructed HOST SRAM stays private; never import these files into a guest.
PRE_CARGO_OLD_ROOT ?= /tmp/sc64-scourge-permissions-base
PRE_CARGO_OUT ?= /tmp/sc64-pre-cargo-egg-compat
PRE_CARGO_COMMON = $(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable $(ZERG_Z7_CANONICAL_FLAGS)
.PHONY: pre-cargo-campaign-build
pre-cargo-campaign-build:
	$(CLANG) $(PRE_CARGO_COMMON) -USC64_WORKER_CARGO_KIND -DSC64_COMPAT_WRITER '-DSC64_COMPAT_GAME_SOURCE="$(PRE_CARGO_OLD_ROOT)/starcraft/game.c"' tests/sc64_pre_cargo_campaign_compat.c tests/sc64_pre_cargo_compat_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $(PRE_CARGO_OUT)/campaign-old-writer
	$(CLANG) $(PRE_CARGO_COMMON) tests/sc64_pre_cargo_campaign_compat.c tests/sc64_pre_cargo_compat_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $(PRE_CARGO_OUT)/campaign-normal-reader
	$(CLANG) $(PRE_CARGO_COMMON) -DSC64_SHARED_EGG_CUSTOM_SERVICE tests/sc64_pre_cargo_campaign_compat.c tests/sc64_pre_cargo_compat_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $(PRE_CARGO_OUT)/campaign-optin-reader
.PHONY: custom-egg-context-terran custom-egg-context-z7 custom-egg-context-z9 custom-egg-context-p10 custom-egg-context-showcase
custom-egg-context-terran:
	$(CLANG) $(CUSTOM_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_context_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-context-terran
	$(COMBINED_BIN_DIR)/custom-egg-context-terran
custom-egg-context-z7:
	$(CLANG) $(CUSTOM_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_context_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-context-z7
	$(COMBINED_BIN_DIR)/custom-egg-context-z7
custom-egg-context-z9:
	$(CLANG) $(CUSTOM_HOST) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_egg_context_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-context-z9
	$(COMBINED_BIN_DIR)/custom-egg-context-z9
custom-egg-context-p10:
	$(CLANG) $(CUSTOM_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(CUSTOM_FACT_DIR),$(PROTOSS_P10_FLAGS)) tests/sc64_shared_egg_context_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-context-p10
	$(COMBINED_BIN_DIR)/custom-egg-context-p10
custom-egg-context-showcase:
	$(CLANG) $(CUSTOM_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_context_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-context-showcase
	$(COMBINED_BIN_DIR)/custom-egg-context-showcase

# Detached HOST candidate only; no Scarab journal tag or writer is admitted.
.PHONY: custom-egg-scarab-detached-terran
custom-egg-scarab-detached-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE $(TERRAN_ROM_FLAGS) tests/sc64_scarab_egg_detached_context.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/custom-egg-scarab-detached-terran
	$(COMBINED_BIN_DIR)/custom-egg-scarab-detached-terran

# Predecessor is the pinned base worktree; the current reader uses stable roster.
SCARAB_PRIOR_ROOT ?= /tmp/sc64-shared-egg-release-publication-20261106
.PHONY: custom-egg-scarab-prior-terran
custom-egg-scarab-prior-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_PRIOR_WRITER '-DSC64_PRIOR_GAME="$(SCARAB_PRIOR_ROOT)/starcraft/game.c"' $(TERRAN_ROM_FLAGS) tests/sc64_scarab_prior_egg_compat.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/scarab-prior-writer
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE $(TERRAN_ROM_FLAGS) tests/sc64_scarab_prior_egg_compat.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/scarab-current-reader
	$(COMBINED_BIN_DIR)/scarab-prior-writer $(COMBINED_BIN_DIR)/prior-egg.sram
	$(COMBINED_BIN_DIR)/scarab-current-reader $(COMBINED_BIN_DIR)/prior-egg.sram
