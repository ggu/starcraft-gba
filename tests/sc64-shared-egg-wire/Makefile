include Makefile
EGG_TEST_BIN_DIR ?= /tmp
.PHONY: shared-egg-identities
EGG_HOST = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_SHARED_EGG_WIRE_HOST_STAGE
EGG_EXTRA = common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
shared-egg-identities:
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-identity-terran
	$(EGG_TEST_BIN_DIR)/shared-egg-identity-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-identity-zerg
	$(EGG_TEST_BIN_DIR)/shared-egg-identity-zerg
	$(CLANG) $(EGG_HOST) $(PROTOSS_P10_BASE_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-identity-protoss
	$(EGG_TEST_BIN_DIR)/shared-egg-identity-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-identity-showcase
	$(EGG_TEST_BIN_DIR)/shared-egg-identity-showcase

EGG_P10_FACT_DIR = .local/shared-egg-stage-p10-facts
.PHONY: shared-egg-stage-facts shared-egg-identities-stage-facts
shared-egg-stage-facts:
	mkdir -p $(EGG_P10_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -DSC64_SHARED_EGG_WIRE_HOST_STAGE $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o $(EGG_P10_FACT_DIR)/generator
	$(EGG_P10_FACT_DIR)/generator > $(EGG_P10_FACT_DIR)/sc64_fact_tables_generated.h
shared-egg-identities-stage-facts: shared-egg-stage-facts
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-facts-terran
	$(EGG_TEST_BIN_DIR)/shared-egg-facts-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-facts-zerg
	$(EGG_TEST_BIN_DIR)/shared-egg-facts-zerg
	$(CLANG) $(EGG_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(EGG_P10_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS)) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-facts-protoss
	$(EGG_TEST_BIN_DIR)/shared-egg-facts-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-facts-showcase
	$(EGG_TEST_BIN_DIR)/shared-egg-facts-showcase
.PHONY: shared-egg-actor-stage-facts
shared-egg-actor-stage-facts: shared-egg-stage-facts
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_actor_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-actor-terran
	$(EGG_TEST_BIN_DIR)/shared-egg-actor-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_actor_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-actor-zerg
	$(EGG_TEST_BIN_DIR)/shared-egg-actor-zerg
	$(CLANG) $(EGG_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(EGG_P10_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS)) tests/sc64_shared_egg_actor_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-actor-protoss
	$(EGG_TEST_BIN_DIR)/shared-egg-actor-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_actor_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-actor-showcase
	$(EGG_TEST_BIN_DIR)/shared-egg-actor-showcase
.PHONY: shared-egg-wire-stage-facts
shared-egg-wire-stage-facts: shared-egg-stage-facts
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_wire_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-wire-terran
	$(EGG_TEST_BIN_DIR)/shared-egg-wire-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_wire_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-wire-zerg
	$(EGG_TEST_BIN_DIR)/shared-egg-wire-zerg
	$(CLANG) $(EGG_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(EGG_P10_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS)) tests/sc64_shared_egg_wire_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-wire-protoss
	$(EGG_TEST_BIN_DIR)/shared-egg-wire-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_wire_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-wire-showcase
	$(EGG_TEST_BIN_DIR)/shared-egg-wire-showcase
.PHONY: shared-egg-auth-real
shared-egg-auth-real: shared-egg-stage-facts
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_stage_save_auth_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-auth-terran
	$(EGG_TEST_BIN_DIR)/shared-egg-auth-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_stage_save_auth_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-auth-zerg
	$(EGG_TEST_BIN_DIR)/shared-egg-auth-zerg
	$(CLANG) $(EGG_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(EGG_P10_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS)) tests/sc64_stage_save_auth_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-auth-protoss
	$(EGG_TEST_BIN_DIR)/shared-egg-auth-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_stage_save_auth_real.c $(EGG_EXTRA) -o $(EGG_TEST_BIN_DIR)/shared-egg-auth-showcase
	$(EGG_TEST_BIN_DIR)/shared-egg-auth-showcase
