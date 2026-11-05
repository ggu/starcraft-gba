include Makefile
.PHONY: shared-egg-identities
EGG_HOST = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_SHARED_EGG_WIRE_HOST_STAGE
EGG_EXTRA = common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
shared-egg-identities:
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-terran
	/tmp/shared-egg-identity-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-zerg
	/tmp/shared-egg-identity-zerg
	$(CLANG) $(EGG_HOST) $(PROTOSS_P10_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-protoss
	/tmp/shared-egg-identity-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-identity-showcase
	/tmp/shared-egg-identity-showcase

EGG_P10_FACT_DIR = .local/shared-egg-stage-p10-facts
.PHONY: shared-egg-stage-facts shared-egg-identities-stage-facts
shared-egg-stage-facts:
	mkdir -p $(EGG_P10_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -DSC64_SHARED_EGG_WIRE_HOST_STAGE $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o $(EGG_P10_FACT_DIR)/generator
	$(EGG_P10_FACT_DIR)/generator > $(EGG_P10_FACT_DIR)/sc64_fact_tables_generated.h
shared-egg-identities-stage-facts: shared-egg-stage-facts
	$(CLANG) $(EGG_HOST) $(TERRAN_ROM_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-facts-terran
	/tmp/shared-egg-facts-terran
	$(CLANG) $(EGG_HOST) $(ZERG_Z7_CANONICAL_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-facts-zerg
	/tmp/shared-egg-facts-zerg
	$(CLANG) $(EGG_HOST) $(patsubst -I$(P10_FACT_V2_DIR),-I$(EGG_P10_FACT_DIR),$(PROTOSS_P10_FLAGS)) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-facts-protoss
	/tmp/shared-egg-facts-protoss
	$(CLANG) $(EGG_HOST) $(SHOWCASE_FLAGS) tests/sc64_shared_egg_identity_real.c $(EGG_EXTRA) -o /tmp/shared-egg-facts-showcase
	/tmp/shared-egg-facts-showcase
