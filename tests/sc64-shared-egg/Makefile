include Makefile
EGG_TEST_BIN_DIR ?= /tmp
.PHONY: test-shared-egg-host
SHARED_EGG_HOST_FLAGS = $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_SHARED_EGG_HOST_STAGE
test-shared-egg-host: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) tests/sc64_shared_egg_host.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-host-fixture
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-host-fixture
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) tests/sc64_z7_egg_production_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z7-legacy-fixture
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z7-legacy-fixture
.PHONY: test-shared-egg-legacy-raw36
test-shared-egg-legacy-raw36: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_egg_legacy_raw36.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-legacy-raw36
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-legacy-raw36
.PHONY: test-shared-egg-custom-journal
test-shared-egg-custom-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-custom-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-custom-journal
.PHONY: test-zl-detached-candidate-binding
test-zl-detached-candidate-binding: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_zl_candidate_binding.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-zl-detached-candidate-binding
	$(EGG_TEST_BIN_DIR)/sc64-zl-detached-candidate-binding
.PHONY: test-shared-egg-z9-journal
test-shared-egg-z9-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_z9_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z9-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z9-journal
.PHONY: test-shared-egg-z8-journal
test-shared-egg-z8-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_z8_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-journal
.PHONY: test-shared-egg-capacity
test-shared-egg-capacity: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_capacity_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-capacity
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-capacity
.PHONY: test-shared-egg-deferred-journal
test-shared-egg-deferred-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_deferred_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-deferred-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-deferred-journal
.PHONY: test-shared-egg-z8-deferred-journal
test-shared-egg-z8-deferred-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_z8_deferred_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-deferred-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-deferred-journal

# The opt-in guest-capable integration uses the same real scheduler/journal
# fixtures as the isolated prototype, with the real menu producer enabled.
LATE_EGG_HOST_FLAGS = $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_SHARED_EGG_LATE_SERVICE
.PHONY: test-late-egg-z8-journal test-late-egg-z9-journal test-late-egg-capacity test-late-egg-z8-deferred test-late-egg-z9-deferred
test-late-egg-z8-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_z8_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-journal
test-late-egg-z9-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_z9_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-journal
test-late-egg-capacity: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_capacity_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-capacity
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-capacity
test-late-egg-z8-deferred: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_z8_deferred_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-deferred
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-deferred
test-late-egg-z9-deferred: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_deferred_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-deferred
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-deferred
.PHONY: test-late-egg-z10-journal test-late-egg-custom-journal
test-late-egg-z10-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) -DSC64_SHARED_EGG_LATE_SERVICE tests/sc64_shared_egg_z10_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z10-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z10-journal
test-late-egg-custom-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-journal
# Use HOST-only predecessor fixtures generated by the documented compatibility
# harness; never import these bytes into a guest.
LATE_EGG_PRE_CARGO_DIR ?= /tmp/sc64-late-egg-pre-cargo-z9
LATE_EGG_PRE_CARGO_OLD_ROOT ?= /tmp/sc64-scourge-permissions-base
LATE_EGG_PRE_CARGO_EXTRA = tests/sc64_pre_cargo_compat_shim.c $(filter-out starcraft/game.c,$(STARCRAFT))
.PHONY: test-late-egg-pre-cargo
test-late-egg-pre-cargo: | $(LATE_ZERG_SIGHT_INPUTS)
	mkdir -p $(LATE_EGG_PRE_CARGO_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -USC64_WORKER_CARGO_KIND -DSC64_COMPAT_WRITER '-DSC64_COMPAT_GAME_SOURCE="$(LATE_EGG_PRE_CARGO_OLD_ROOT)/starcraft/game.c"' tests/sc64_pre_cargo_campaign_compat.c $(LATE_EGG_PRE_CARGO_EXTRA) -o $(LATE_EGG_PRE_CARGO_DIR)/old-writer
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_pre_cargo_campaign_compat.c $(LATE_EGG_PRE_CARGO_EXTRA) -o $(LATE_EGG_PRE_CARGO_DIR)/normal-reader
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_pre_cargo_campaign_compat.c $(LATE_EGG_PRE_CARGO_EXTRA) -o $(LATE_EGG_PRE_CARGO_DIR)/late-reader
	@for mission in 6 7 8 9; do \
	    $(LATE_EGG_PRE_CARGO_DIR)/old-writer $(LATE_EGG_PRE_CARGO_DIR)/z$$mission.sav $$mission && \
	    $(LATE_EGG_PRE_CARGO_DIR)/normal-reader $(LATE_EGG_PRE_CARGO_DIR)/z$$mission.sav $$mission && \
	    $(LATE_EGG_PRE_CARGO_DIR)/late-reader $(LATE_EGG_PRE_CARGO_DIR)/z$$mission.sav $$mission || exit $$?; \
	done
.PHONY: test-late-egg-custom-input
test-late-egg-custom-input: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_ordinary_input.c tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-input
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-input

# Experimental Z1 admission, not a release product flag.
.PHONY: test-z1-egg-host-journal
test-z1-egg-host-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_Z1_HOST_STAGE tests/sc64_shared_egg_z1_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z1-egg-journal
	$(EGG_TEST_BIN_DIR)/sc64-z1-egg-journal

# Synthetic permission test; not a qualification of the private PUNI bank.
.PHONY: test-z2-egg-synthetic-journal
test-z2-egg-synthetic-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_Z2_HOST_STAGE -DSC64_EARLY_ZERG_PERMISSIONS -DSC64_EARLY_ZERG_PERMISSION_TEST_FIXTURE -Itests/fixtures/early-zerg-synthetic tests/sc64_shared_egg_z2_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z2-egg-synthetic
	$(EGG_TEST_BIN_DIR)/sc64-z2-egg-synthetic

# This opt-in test requires the separately prepared owned-ROM permission bank.
SC64_EARLY_PERMISSIONS_DIR ?= .local/sc64/early-permissions
.PHONY: test-z2-egg-actual-journal
test-z2-egg-actual-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_Z2_HOST_STAGE -DSC64_EARLY_ZERG_PERMISSIONS -DSC64_Z2_ACTUAL_PERMISSIONS -I$(SC64_EARLY_PERMISSIONS_DIR) tests/sc64_shared_egg_z2_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z2-egg-actual
	$(EGG_TEST_BIN_DIR)/sc64-z2-egg-actual

.PHONY: test-z3-egg-actual-journal
test-z3-egg-actual-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(LATE_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_Z3_HOST_STAGE -DSC64_EARLY_ZERG_PERMISSIONS -DSC64_Z3_ACTUAL_PERMISSIONS -I$(SC64_EARLY_PERMISSIONS_DIR) tests/sc64_shared_egg_z3_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z3-egg-actual
	$(EGG_TEST_BIN_DIR)/sc64-z3-egg-actual

Z4_EGG_HOST_FLAGS = $(LATE_EGG_HOST_FLAGS) -DSC64_Z4_DATA_ENABLED -DSC64_Z4_ENABLED -DSC64_Z4_RUNTIME -DSC64_Z4_CAMPAIGN_ENTRY -I.local/sc64/z4/generated -DSC64_SHARED_EGG_Z4_HOST_STAGE -DSC64_EARLY_ZERG_PERMISSIONS -DSC64_Z4_ACTUAL_PERMISSIONS -I$(SC64_EARLY_PERMISSIONS_DIR)
.PHONY: test-z4-egg-actual-journal
test-z4-egg-actual-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(Z4_EGG_HOST_FLAGS) tests/sc64_shared_egg_z4_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z4-egg-actual
	$(EGG_TEST_BIN_DIR)/sc64-z4-egg-actual
.PHONY: test-z4-egg-retry-build
test-z4-egg-retry-build: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(Z4_EGG_HOST_FLAGS) tests/sc64_shared_egg_z4_retry_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z4-egg-retry
.PHONY: test-z4-egg-synthetic-journal
test-z4-egg-synthetic-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(filter-out -DSC64_Z4_ACTUAL_PERMISSIONS -I$(SC64_EARLY_PERMISSIONS_DIR),$(Z4_EGG_HOST_FLAGS)) -DSC64_EARLY_ZERG_PERMISSION_TEST_FIXTURE -Itests/fixtures/early-zerg-z4-synthetic tests/sc64_shared_egg_z4_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z4-egg-synthetic
	$(EGG_TEST_BIN_DIR)/sc64-z4-egg-synthetic

SC64_MID_PERMISSIONS_DIR ?= .local/sc64/mid-permissions
Z6_EGG_HOST_FLAGS = $(LATE_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_Z6_HOST_STAGE -DSC64_MID_ZERG_PERMISSIONS -DSC64_Z6_ACTUAL_PERMISSIONS -I$(SC64_MID_PERMISSIONS_DIR)
.PHONY: test-z6-egg-actual-journal
test-z6-egg-actual-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(Z6_EGG_HOST_FLAGS) tests/sc64_shared_egg_z6_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z6-egg-actual
	$(EGG_TEST_BIN_DIR)/sc64-z6-egg-actual
.PHONY: test-z6-egg-synthetic-journal
test-z6-egg-synthetic-journal: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(filter-out -DSC64_Z6_ACTUAL_PERMISSIONS -I$(SC64_MID_PERMISSIONS_DIR),$(Z6_EGG_HOST_FLAGS)) -DSC64_MID_ZERG_PERMISSION_TEST_FIXTURE -Itests/fixtures/mid-zerg-synthetic tests/sc64_shared_egg_z6_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z6-egg-synthetic
	$(EGG_TEST_BIN_DIR)/sc64-z6-egg-synthetic
.PHONY: test-z6-egg-capacity
test-z6-egg-capacity: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(Z6_EGG_HOST_FLAGS) tests/sc64_shared_egg_z6_capacity_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z6-egg-capacity
	$(EGG_TEST_BIN_DIR)/sc64-z6-egg-capacity
.PHONY: test-z5-egg-legacy-reader-build
test-z5-egg-legacy-reader-build: | $(LATE_ZERG_SIGHT_INPUTS)
	$(CLANG) $(Z6_EGG_HOST_FLAGS) tests/sc64_z5_egg_legacy_continuation.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-z5-egg-legacy-reader
