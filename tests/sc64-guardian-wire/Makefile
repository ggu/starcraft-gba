include Makefile
GUARD_TEST_BIN_DIR ?= /tmp
.PHONY: guardian-real
GUARD_HOST = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_GUARDIAN_COCOON_STAGE
GUARD_EXTRA = common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
.PHONY: guardian-legacy-egg-context
guardian-legacy-egg-context:
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_legacy_egg_context.c tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(GUARD_TEST_BIN_DIR)/guardian-legacy-egg-context
	$(GUARD_TEST_BIN_DIR)/guardian-legacy-egg-context
guardian-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-terran
	$(GUARD_TEST_BIN_DIR)/guardian-real-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-zerg
	$(GUARD_TEST_BIN_DIR)/guardian-real-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-protoss
	$(GUARD_TEST_BIN_DIR)/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-showcase
	$(GUARD_TEST_BIN_DIR)/guardian-real-showcase
.PHONY: guardian-real-remaining
guardian-real-remaining:
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-protoss
	$(GUARD_TEST_BIN_DIR)/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-showcase
	$(GUARD_TEST_BIN_DIR)/guardian-real-showcase
.PHONY: guardian-real-zps
guardian-real-zps:
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-zerg
	$(GUARD_TEST_BIN_DIR)/guardian-real-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-protoss
	$(GUARD_TEST_BIN_DIR)/guardian-real-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_wire_real.c $(GUARD_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-real-showcase
	$(GUARD_TEST_BIN_DIR)/guardian-real-showcase
.PHONY: guardian-fact-generator
guardian-fact-generator:
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o /tmp/guardian-fact-gen-off
	/tmp/guardian-fact-gen-off > /tmp/guardian-facts-off.h
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT -DSC64_GUARDIAN_COCOON_STAGE tools/sc64_fact_tables_gen.c -o /tmp/guardian-fact-gen-on
	/tmp/guardian-fact-gen-on > /tmp/guardian-facts-on.h
	cmp /tmp/guardian-facts-off.h /tmp/guardian-facts-on.h
.PHONY: guardian-journal-real
GUARD_JOURNAL_EXTRA = tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
guardian-journal-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-terran
	$(GUARD_TEST_BIN_DIR)/guardian-journal-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-zerg
	$(GUARD_TEST_BIN_DIR)/guardian-journal-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-protoss
	$(GUARD_TEST_BIN_DIR)/guardian-journal-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-showcase
	$(GUARD_TEST_BIN_DIR)/guardian-journal-showcase
.PHONY: guardian-lifecycle-real
guardian-lifecycle-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_lifecycle_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-lifecycle-terran
	/tmp/guardian-lifecycle-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_lifecycle_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-lifecycle-zerg
	/tmp/guardian-lifecycle-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_lifecycle_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-lifecycle-protoss
	/tmp/guardian-lifecycle-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_lifecycle_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-lifecycle-showcase
	/tmp/guardian-lifecycle-showcase
.PHONY: guardian-frame-real
guardian-frame-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-terran
	/tmp/guardian-frame-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-zerg
	/tmp/guardian-frame-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-protoss
	/tmp/guardian-frame-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-showcase
	/tmp/guardian-frame-showcase
.PHONY: guardian-render-game-fake guardian-frame-render-real
guardian-render-game-fake:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) -DSC64_GUARDIAN_COCOON_RENDER_STAGE -DSC64_GUARDIAN_TEST_BANK_HEADER='"sc64_guardian_cocoon_fake_bank.h"' -Itests/guardian-render-fake tests/sc64_guardian_render_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-render-game-fake
	/tmp/guardian-render-game-fake
guardian-frame-render-real:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) -DSC64_GUARDIAN_COCOON_RENDER_STAGE -I.local/sc64/guardian-cocoon/bank tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-render-terran
	/tmp/guardian-frame-render-terran
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) -DSC64_GUARDIAN_COCOON_RENDER_STAGE -I.local/sc64/guardian-cocoon/bank tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-render-zerg
	/tmp/guardian-frame-render-zerg
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) -DSC64_GUARDIAN_COCOON_RENDER_STAGE -I.local/sc64/guardian-cocoon/bank tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-render-protoss
	/tmp/guardian-frame-render-protoss
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) -DSC64_GUARDIAN_COCOON_RENDER_STAGE -I.local/sc64/guardian-cocoon/bank tests/sc64_guardian_frame_game.c $(GUARD_JOURNAL_EXTRA) -o /tmp/guardian-frame-render-showcase
	/tmp/guardian-frame-render-showcase

## Individually guarded profile slices.
.PHONY: guardian-journal-slice-terran
guardian-journal-slice-terran:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-terran
	$(GUARD_TEST_BIN_DIR)/guardian-journal-terran
.PHONY: guardian-journal-slice-zerg
guardian-journal-slice-zerg:
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-zerg
	$(GUARD_TEST_BIN_DIR)/guardian-journal-zerg
.PHONY: guardian-journal-slice-protoss
guardian-journal-slice-protoss:
	$(CLANG) $(GUARD_HOST) $(patsubst -Ibuild/generated/p10-fact-v2,-Ibuild/generated/guardian-p10-fact,$(PROTOSS_P10_FLAGS)) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-protoss
	$(GUARD_TEST_BIN_DIR)/guardian-journal-protoss
.PHONY: guardian-journal-slice-showcase
guardian-journal-slice-showcase:
	$(CLANG) $(GUARD_HOST) $(SHOWCASE_FLAGS) tests/sc64_guardian_journal_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-journal-showcase
	$(GUARD_TEST_BIN_DIR)/guardian-journal-showcase
.PHONY: guardian-campaign-construction
# Constructed HOST world only. The paid building marker has no authenticated
# journal namespace yet and this target does not qualify SRAM or guest use.
guardian-campaign-construction:
	$(CLANG) $(GUARD_HOST) -DSC64_GUARDIAN_CAMPAIGN_CONSTRUCTION_STAGE $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_campaign_construction_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-campaign-construction
	$(GUARD_TEST_BIN_DIR)/guardian-campaign-construction

# A separate private fact path for the combined HOST roster; it never touches
# the pinned P10 inputs or the normal/Egg-only generated tables.
GUARD_FULL_FACT_DIR ?= .local/guardian-full-facts-stage
GUARD_FULL_FACT_FLAGS = -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_FULL_FACTS_STAGE
GUARD_FULL_P10_FLAGS = $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT
.PHONY: guardian-full-facts
guardian-full-facts:
	mkdir -p $(GUARD_FULL_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(GUARD_FULL_FACT_FLAGS) $(GUARD_FULL_P10_FLAGS) tools/sc64_fact_tables_gen.c -o $(GUARD_FULL_FACT_DIR)/generator
	$(GUARD_FULL_FACT_DIR)/generator > $(GUARD_FULL_FACT_DIR)/sc64_fact_tables_generated.h
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -I$(GUARD_FULL_FACT_DIR) $(GUARD_FULL_FACT_FLAGS) $(GUARD_FULL_P10_FLAGS) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-full-facts
	$(GUARD_TEST_BIN_DIR)/guardian-full-facts
# A second reference roster uses the appended, distinct Guardian identity.
GUARD_FULL_DISTINCT_FLAGS = -DGBA_HOST -DSC64_RACES -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_FACT_SIGHT_TABLES $(GUARD_FULL_FACT_FLAGS)
.PHONY: guardian-full-facts-distinct
guardian-full-facts-distinct:
	mkdir -p $(GUARD_FULL_FACT_DIR)/distinct
	$(CLANG) -std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Icommon -Istarcraft $(GUARD_FULL_DISTINCT_FLAGS) tools/sc64_fact_tables_gen.c -o $(GUARD_FULL_FACT_DIR)/distinct/generator
	$(GUARD_FULL_FACT_DIR)/distinct/generator > $(GUARD_FULL_FACT_DIR)/distinct/sc64_fact_tables_generated.h
	$(CLANG) -std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Icommon -Istarcraft -I$(GUARD_FULL_FACT_DIR)/distinct $(GUARD_FULL_DISTINCT_FLAGS) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-full-facts-distinct
	$(GUARD_TEST_BIN_DIR)/guardian-full-facts-distinct
.PHONY: guardian-campaign-permissions
guardian-campaign-permissions:
	$(CLANG) $(GUARD_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_campaign_permissions_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-campaign-permissions
	$(GUARD_TEST_BIN_DIR)/guardian-campaign-permissions
.PHONY: guardian-candidate-validation
guardian-candidate-validation:
	$(CLANG) $(GUARD_HOST) -DSC64_GUARDIAN_CAMPAIGN_CONSTRUCTION_STAGE $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_candidate_validation_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-candidate-validation
	$(GUARD_TEST_BIN_DIR)/guardian-candidate-validation
.PHONY: guardian-candidate-distinct
guardian-candidate-distinct:
	$(CLANG) $(GUARD_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_guardian_candidate_distinct_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-candidate-distinct
	$(GUARD_TEST_BIN_DIR)/guardian-candidate-distinct

# Full Egg-first + Guardian + Scarab roster facts. These private tables test
# actor facts only; unsupported typed/derived journals and guest remain fenced.
GUARD_COMBINED_FACT_DIR ?= .local/guardian-combined-full-facts-stage
GUARD_COMBINED_FACT_FLAGS = -DSC64_SHARED_EGG_CUSTOM_SERVICE -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE -DSC64_GUARDIAN_FULL_FACTS_STAGE
.PHONY: guardian-combined-full-facts-p10 guardian-combined-full-facts-z7 guardian-combined-full-facts-distinct
guardian-combined-full-facts-p10:
	mkdir -p $(GUARD_COMBINED_FACT_DIR)/p10
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_FULL_P10_FLAGS) tools/sc64_fact_tables_gen.c -o $(GUARD_COMBINED_FACT_DIR)/p10/generator
	$(GUARD_COMBINED_FACT_DIR)/p10/generator > $(GUARD_COMBINED_FACT_DIR)/p10/sc64_fact_tables_generated.h
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -I$(GUARD_COMBINED_FACT_DIR)/p10 $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_FULL_P10_FLAGS) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-combined-facts-p10
	$(GUARD_TEST_BIN_DIR)/guardian-combined-facts-p10
guardian-combined-full-facts-z7:
	mkdir -p $(GUARD_COMBINED_FACT_DIR)/z7
	$(CLANG) $(GUARD_HOST) -Istarcraft $(GUARD_COMBINED_FACT_FLAGS) $(filter-out -DSC64_FACT_TABLES,$(ZERG_Z7_CANONICAL_FLAGS)) tools/sc64_fact_tables_gen.c -o $(GUARD_COMBINED_FACT_DIR)/z7/generator
	$(GUARD_COMBINED_FACT_DIR)/z7/generator > $(GUARD_COMBINED_FACT_DIR)/z7/sc64_fact_tables_generated.h
	$(CLANG) $(GUARD_HOST) -Istarcraft -I$(GUARD_COMBINED_FACT_DIR)/z7 $(GUARD_COMBINED_FACT_FLAGS) $(filter-out -DSC64_FACT_TABLES,$(ZERG_Z7_CANONICAL_FLAGS)) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-combined-facts-z7
	$(GUARD_TEST_BIN_DIR)/guardian-combined-facts-z7
guardian-combined-full-facts-distinct:
	mkdir -p $(GUARD_COMBINED_FACT_DIR)/distinct
	$(CLANG) -std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Icommon -Istarcraft -DGBA_HOST -DSC64_RACES -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_FACT_SIGHT_TABLES $(GUARD_COMBINED_FACT_FLAGS) tools/sc64_fact_tables_gen.c -o $(GUARD_COMBINED_FACT_DIR)/distinct/generator
	$(GUARD_COMBINED_FACT_DIR)/distinct/generator > $(GUARD_COMBINED_FACT_DIR)/distinct/sc64_fact_tables_generated.h
	$(CLANG) -std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Icommon -Istarcraft -I$(GUARD_COMBINED_FACT_DIR)/distinct -DGBA_HOST -DSC64_RACES -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_FACT_SIGHT_TABLES $(GUARD_COMBINED_FACT_FLAGS) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-combined-facts-distinct
	$(GUARD_TEST_BIN_DIR)/guardian-combined-facts-distinct

# Feature-15 packed Scarab motion capability: isolated private fact paths.
GUARD_COMBINED_FLIGHT_FLAGS = -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE
.PHONY: guardian-combined-flight-facts-p10 guardian-combined-flight-facts-z7 guardian-combined-flight-facts-distinct
guardian-combined-flight-facts-p10:
	mkdir -p $(GUARD_COMBINED_FACT_DIR)/flight-p10
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_COMBINED_FLIGHT_FLAGS) $(GUARD_FULL_P10_FLAGS) tools/sc64_fact_tables_gen.c -o $(GUARD_COMBINED_FACT_DIR)/flight-p10/generator
	$(GUARD_COMBINED_FACT_DIR)/flight-p10/generator > $(GUARD_COMBINED_FACT_DIR)/flight-p10/sc64_fact_tables_generated.h
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -I$(GUARD_COMBINED_FACT_DIR)/flight-p10 $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_COMBINED_FLIGHT_FLAGS) $(GUARD_FULL_P10_FLAGS) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-combined-flight-facts-p10
	$(GUARD_TEST_BIN_DIR)/guardian-combined-flight-facts-p10
guardian-combined-flight-facts-z7:
	mkdir -p $(GUARD_COMBINED_FACT_DIR)/flight-z7
	$(CLANG) $(GUARD_HOST) -Istarcraft $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_COMBINED_FLIGHT_FLAGS) $(filter-out -DSC64_FACT_TABLES,$(ZERG_Z7_CANONICAL_FLAGS)) tools/sc64_fact_tables_gen.c -o $(GUARD_COMBINED_FACT_DIR)/flight-z7/generator
	$(GUARD_COMBINED_FACT_DIR)/flight-z7/generator > $(GUARD_COMBINED_FACT_DIR)/flight-z7/sc64_fact_tables_generated.h
	$(CLANG) $(GUARD_HOST) -Istarcraft -I$(GUARD_COMBINED_FACT_DIR)/flight-z7 $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_COMBINED_FLIGHT_FLAGS) $(filter-out -DSC64_FACT_TABLES,$(ZERG_Z7_CANONICAL_FLAGS)) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-combined-flight-facts-z7
	$(GUARD_TEST_BIN_DIR)/guardian-combined-flight-facts-z7
guardian-combined-flight-facts-distinct:
	mkdir -p $(GUARD_COMBINED_FACT_DIR)/flight-distinct
	$(CLANG) -std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Icommon -Istarcraft -DGBA_HOST -DSC64_RACES -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_FACT_SIGHT_TABLES $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_COMBINED_FLIGHT_FLAGS) tools/sc64_fact_tables_gen.c -o $(GUARD_COMBINED_FACT_DIR)/flight-distinct/generator
	$(GUARD_COMBINED_FACT_DIR)/flight-distinct/generator > $(GUARD_COMBINED_FACT_DIR)/flight-distinct/sc64_fact_tables_generated.h
	$(CLANG) -std=c11 -O1 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Icommon -Istarcraft -I$(GUARD_COMBINED_FACT_DIR)/flight-distinct -DGBA_HOST -DSC64_RACES -DSC64_ZP_CAMPAIGN_ACTORS -DSC64_FACT_SIGHT_TABLES $(GUARD_COMBINED_FACT_FLAGS) $(GUARD_COMBINED_FLIGHT_FLAGS) -DSC64_FACT_TABLES tests/sc64_guardian_full_facts.c -o $(GUARD_TEST_BIN_DIR)/guardian-combined-flight-facts-distinct
	$(GUARD_TEST_BIN_DIR)/guardian-combined-flight-facts-distinct
# Explicit staged early-permission input; normal flags remain unchanged.
GUARD_EARLY_PERMISSION_DIR ?= .local/guardian-early-permissions-stage
.PHONY: guardian-early-permissions
guardian-early-permissions:
	$(CLANG) $(GUARD_HOST) -DSC64_EARLY_ZERG_PERMISSIONS -I$(GUARD_EARLY_PERMISSION_DIR) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_guardian_early_permissions_game.c $(GUARD_JOURNAL_EXTRA) -o $(GUARD_TEST_BIN_DIR)/guardian-early-permissions
	$(GUARD_TEST_BIN_DIR)/guardian-early-permissions
