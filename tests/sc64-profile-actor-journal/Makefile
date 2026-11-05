# Isolated HOST-only selected journal probe; use with `make -f Makefile -f ...`.
PROFILE_PRIOR_ROOT ?= /tmp/sc64-shared-egg-release-publication-20261106
PROFILE_FACT_ROOT ?= /tmp/sc64-guardian-combined-facts-nov6/.local/guardian-combined-full-facts-stage
PROFILE_OLD_P10_FACT ?= build/generated/p10-shared-egg-fact
PROFILE_OUT ?= /tmp/sc64-profile-actor-journal
PROFILE_STAGE = -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_STABLE_ROSTER_HOST_STAGE -DSC64_SHARED_ACTOR_LAYOUT_HOST_STAGE -DSC64_GUARDIAN_COCOON_STAGE -DSC64_GUARDIAN_EGG_CANDIDATE_HOST_STAGE -DSC64_SHARED_ACTOR_FULL_HOST_STAGE -DSC64_SHARED_ACTOR_FLIGHT_HOST_STAGE -DSC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_SCARAB_HUMAN_CONTROL_PROBE
PROFILE_HOST = -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-misleading-indentation -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_SHARED_EGG_CUSTOM_SERVICE
PROFILE_EXTRA = tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
PROFILE_Z7 = $(ZERG_Z7_CANONICAL_FLAGS) -DSC64_Z7_DERIVED_SAVE
PROFILE_BC = $(BC_FLAGS)
PROFILE_P10_OLD = $(patsubst -I$(P10_FACT_V2_DIR),-I$(PROFILE_OLD_P10_FACT),$(PROTOSS_P10_FLAGS))
PROFILE_P10_NEW = -DSC64_GUARDIAN_FULL_FACTS_STAGE $(patsubst -I$(P10_FACT_V2_DIR),-I$(PROFILE_FACT_ROOT)/flight-p10,$(PROTOSS_P10_FLAGS))
.PHONY: profile-actor-z7 profile-actor-p10
profile-actor-z7:
	mkdir -p $(PROFILE_OUT)/z7
	$(CLANG) $(PROFILE_HOST) $(PROFILE_BC) -DSC64_PRIOR_WRITER '-DSC64_PRIOR_GAME="$(PROFILE_PRIOR_ROOT)/starcraft/game.c"' $(PROFILE_Z7) tests/sc64_scarab_prior_egg_compat.c $(PROFILE_EXTRA) -o $(PROFILE_OUT)/z7/writer
	$(PROFILE_OUT)/z7/writer $(PROFILE_OUT)/z7/prior.sram
	$(CLANG) $(PROFILE_HOST) $(PROFILE_BC) $(PROFILE_STAGE) $(PROFILE_Z7) tests/sc64_profile_actor_journal.c $(PROFILE_EXTRA) -o $(PROFILE_OUT)/z7/guardian
	$(PROFILE_OUT)/z7/guardian $(PROFILE_OUT)/z7/prior.sram
profile-actor-p10:
	mkdir -p $(PROFILE_OUT)/p10
	$(CLANG) $(PROFILE_HOST) $(PROFILE_BC) -DSC64_PRIOR_WRITER '-DSC64_PRIOR_GAME="$(PROFILE_PRIOR_ROOT)/starcraft/game.c"' $(PROFILE_P10_OLD) tests/sc64_scarab_prior_egg_compat.c $(PROFILE_EXTRA) -o $(PROFILE_OUT)/p10/writer
	$(PROFILE_OUT)/p10/writer $(PROFILE_OUT)/p10/prior.sram
	$(CLANG) $(PROFILE_HOST) $(PROFILE_BC) $(PROFILE_STAGE) $(PROFILE_P10_NEW) tests/sc64_profile_actor_journal.c $(PROFILE_EXTRA) -o $(PROFILE_OUT)/p10/guardian
	$(PROFILE_OUT)/p10/guardian $(PROFILE_OUT)/p10/prior.sram
