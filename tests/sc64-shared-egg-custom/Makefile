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

# HOST only: supply SRAM from a genuinely compiled prior writer.
.PHONY: custom-egg-scarab-selected-terran
custom-egg-scarab-selected-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_SHARED_ACTOR_JOURNAL_HOST_STAGE $(TERRAN_ROM_FLAGS) tests/sc64_scarab_egg_selected_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/scarab-selected-journal
	$(COMBINED_BIN_DIR)/scarab-selected-journal $(SCARAB_PRIOR_SRAM)

# Genuine predecessor writers: each reader compares every published unit ID,
# raw mapping and name before accepting the old Egg/Reaver/Shuttle SRAM.
define SCARAB_PRIOR_PROFILE
.PHONY: custom-egg-scarab-prior-$(1)
custom-egg-scarab-prior-$(1):
	mkdir -p $(COMBINED_BIN_DIR)/$(1)
	$(CLANG) $(CUSTOM_HOST) -DSC64_PRIOR_WRITER '-DSC64_PRIOR_GAME="$(SCARAB_PRIOR_ROOT)/starcraft/game.c"' $(2) tests/sc64_scarab_prior_egg_compat.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/$(1)/writer
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE $(2) tests/sc64_scarab_prior_egg_compat.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/$(1)/reader
	$(COMBINED_BIN_DIR)/$(1)/writer $(COMBINED_BIN_DIR)/$(1)/prior.sram
	$(COMBINED_BIN_DIR)/$(1)/reader $(COMBINED_BIN_DIR)/$(1)/prior.sram
endef
$(eval $(call SCARAB_PRIOR_PROFILE,z7,$(ZERG_Z7_CANONICAL_FLAGS)))
$(eval $(call SCARAB_PRIOR_PROFILE,z9,$(ZERG_Z9_CANONICAL_FLAGS)))
$(eval $(call SCARAB_PRIOR_PROFILE,showcase,$(SHOWCASE_FLAGS)))

# P10 facts must be generated separately for the predecessor and new roster.
SCARAB_P10_PRIOR_FACT ?= build/scarab-prior-matrix/p10/facts-prior
SCARAB_P10_NEW_FACT ?= build/scarab-prior-matrix/p10/facts-new
SCARAB_P10_GEN_FLAGS = $(filter-out -DSC64_FACT_TABLES -I$(P10_FACT_V2_DIR),$(PROTOSS_P10_FLAGS))
.PHONY: custom-egg-scarab-prior-p10
custom-egg-scarab-prior-p10:
	mkdir -p $(COMBINED_BIN_DIR)/p10 $(SCARAB_P10_PRIOR_FACT) $(SCARAB_P10_NEW_FACT)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SHARED_EGG_WIRE_HOST_STAGE $(SCARAB_P10_GEN_FLAGS) -Istarcraft tools/sc64_fact_tables_gen.c -o $(SCARAB_P10_PRIOR_FACT)/generator
	$(SCARAB_P10_PRIOR_FACT)/generator > $(SCARAB_P10_PRIOR_FACT)/sc64_fact_tables_generated.h
	$(CLANG) $(CUSTOM_HOST) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE $(SCARAB_P10_GEN_FLAGS) -Istarcraft tools/sc64_fact_tables_gen.c -o $(SCARAB_P10_NEW_FACT)/generator
	$(SCARAB_P10_NEW_FACT)/generator > $(SCARAB_P10_NEW_FACT)/sc64_fact_tables_generated.h
	$(CLANG) $(CUSTOM_HOST) -DSC64_PRIOR_WRITER '-DSC64_PRIOR_GAME="$(SCARAB_PRIOR_ROOT)/starcraft/game.c"' $(patsubst -I$(P10_FACT_V2_DIR),-I$(SCARAB_P10_PRIOR_FACT),$(PROTOSS_P10_FLAGS)) tests/sc64_scarab_prior_egg_compat.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/p10/writer
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE $(patsubst -I$(P10_FACT_V2_DIR),-I$(SCARAB_P10_NEW_FACT),$(PROTOSS_P10_FLAGS)) tests/sc64_scarab_prior_egg_compat.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/p10/reader
	$(COMBINED_BIN_DIR)/p10/writer $(COMBINED_BIN_DIR)/p10/prior.sram
	$(COMBINED_BIN_DIR)/p10/reader $(COMBINED_BIN_DIR)/p10/prior.sram
$(eval $(call SCARAB_PRIOR_PROFILE,terran-matrix,$(TERRAN_ROM_FLAGS)))

# Full selected candidate uses separate feature7 identity; HOST Terran only.
.PHONY: custom-shared-actor-full-terran
custom-shared-actor-full-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE $(TERRAN_ROM_FLAGS) tests/sc64_shared_actor_full_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/shared-actor-full
	$(COMBINED_BIN_DIR)/shared-actor-full $(SCARAB_PRIOR_SRAM)

.PHONY: custom-shared-actor-full-z9
custom-shared-actor-full-z9:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_actor_full_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/shared-actor-full-z9
	$(COMBINED_BIN_DIR)/shared-actor-full-z9 $(SCARAB_PRIOR_SRAM)

# Independent old feature5 binary writes the exact predecessor consumed below.
SCARAB_FEATURE5_ROOT ?= /tmp/sc64-scarab-feature5-prior-nov6
.PHONY: custom-shared-actor-feature5-writer
custom-shared-actor-feature5-writer:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_SHARED_ACTOR_JOURNAL_HOST_STAGE '-DSC64_PRIOR_GAME="$(SCARAB_FEATURE5_ROOT)/starcraft/game.c"' '-DSC64_PRIOR_SCARAB_ADAPTER="$(SCARAB_FEATURE5_ROOT)/starcraft/sc64_scarab_unit_adapter.h"' $(TERRAN_ROM_FLAGS) tests/sc64_scarab_feature5_writer.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/feature5-writer
	$(COMBINED_BIN_DIR)/feature5-writer $(SCARAB_PRIOR_SRAM) $(COMBINED_BIN_DIR)/feature5.sram

.PHONY: custom-shared-actor-flight-terran
custom-shared-actor-flight-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE $(TERRAN_ROM_FLAGS) tests/sc64_shared_actor_flight_journal.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/shared-actor-flight
	$(COMBINED_BIN_DIR)/shared-actor-flight $(SCARAB_PRIOR_SRAM)

.PHONY: custom-scarab-nydus-z9
custom-scarab-nydus-z9:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE $(BC_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_scarab_nydus_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/scarab-nydus-z9
	$(COMBINED_BIN_DIR)/scarab-nydus-z9

.PHONY: custom-scarab-full-service-terran
custom-scarab-full-service-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE $(TERRAN_ROM_FLAGS) tests/sc64_scarab_full_service_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/scarab-full-service
	$(COMBINED_BIN_DIR)/scarab-full-service

.PHONY: custom-scarab-tech-path-terran
custom-scarab-tech-path-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_TECH_PATH_HOST_PROBE $(TERRAN_ROM_FLAGS) tests/sc64_scarab_tech_path_game.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/scarab-tech-path
	$(COMBINED_BIN_DIR)/scarab-tech-path

.PHONY: custom-scarab-tech-controller-terran
custom-scarab-tech-controller-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE -DSC64_SCARAB_TECH_PATH_HOST_PROBE $(TERRAN_ROM_FLAGS) tests/sc64_scarab_tech_controller.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/scarab-tech-controller
	$(COMBINED_BIN_DIR)/scarab-tech-controller write $(COMBINED_BIN_DIR)/controller.sram
	$(COMBINED_BIN_DIR)/scarab-tech-controller read $(COMBINED_BIN_DIR)/controller.sram
	$(COMBINED_BIN_DIR)/scarab-tech-controller launch-check $(COMBINED_BIN_DIR)/controller.sram
	$(COMBINED_BIN_DIR)/scarab-tech-controller attack $(COMBINED_BIN_DIR)/controller.sram $(COMBINED_BIN_DIR)/impact.sram
	$(COMBINED_BIN_DIR)/scarab-tech-controller resume $(COMBINED_BIN_DIR)/impact.sram

.PHONY: custom-shared-actor-six-tag-selector
custom-shared-actor-six-tag-selector:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-misleading-indentation -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_WORKER_CARGO_KIND -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE tests/gba_save_prepared.c -o $(COMBINED_BIN_DIR)/six-tag-selector
	$(COMBINED_BIN_DIR)/six-tag-selector

.PHONY: custom-shared-actor-legacy-cargo-terran
custom-shared-actor-legacy-cargo-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE $(TERRAN_ROM_FLAGS) tests/sc64_shared_actor_legacy_cargo_context.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/shared-actor-legacy-cargo
	$(COMBINED_BIN_DIR)/shared-actor-legacy-cargo

SCARAB_PRE_CARGO_ROOT ?= /tmp/sc64-scourge-permissions-base
.PHONY: custom-shared-actor-prior-cargo-terran
custom-shared-actor-prior-cargo-terran:
	mkdir -p $(COMBINED_BIN_DIR)
	$(CLANG) $(filter-out -DSC64_SHARED_EGG_CUSTOM_SERVICE,$(CUSTOM_HOST)) -DSC64_PRIOR_WRITER '-DSC64_PRIOR_GAME="$(SCARAB_PRE_CARGO_ROOT)/starcraft/game.c"' $(TERRAN_ROM_FLAGS) -USC64_WORKER_CARGO_KIND tests/sc64_shared_actor_prior_cargo.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/pre-cargo-writer
	$(CLANG) $(CUSTOM_HOST) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE $(TERRAN_ROM_FLAGS) tests/sc64_shared_actor_prior_cargo.c $(CUSTOM_EXTRA) -o $(COMBINED_BIN_DIR)/pre-cargo-reader
	$(COMBINED_BIN_DIR)/pre-cargo-writer $(COMBINED_BIN_DIR)/pre-cargo.sram
	$(COMBINED_BIN_DIR)/pre-cargo-reader $(COMBINED_BIN_DIR)/pre-cargo.sram
