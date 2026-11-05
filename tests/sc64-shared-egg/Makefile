include Makefile
EGG_TEST_BIN_DIR ?= /tmp
.PHONY: test-shared-egg-host
SHARED_EGG_HOST_FLAGS = $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_SHARED_EGG_HOST_STAGE
test-shared-egg-host:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) tests/sc64_shared_egg_host.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-host-fixture
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-host-fixture
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) tests/sc64_z7_egg_production_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z7-legacy-fixture
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z7-legacy-fixture
.PHONY: test-shared-egg-legacy-raw36
test-shared-egg-legacy-raw36:
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) tests/sc64_shared_egg_legacy_raw36.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-legacy-raw36
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-legacy-raw36
.PHONY: test-shared-egg-custom-journal
test-shared-egg-custom-journal:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-custom-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-custom-journal
.PHONY: test-zl-detached-candidate-binding
test-zl-detached-candidate-binding:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_zl_candidate_binding.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-zl-detached-candidate-binding
	$(EGG_TEST_BIN_DIR)/sc64-zl-detached-candidate-binding
.PHONY: test-shared-egg-z9-journal
test-shared-egg-z9-journal:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_z9_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z9-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z9-journal
.PHONY: test-shared-egg-z8-journal
test-shared-egg-z8-journal:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_z8_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-journal
.PHONY: test-shared-egg-capacity
test-shared-egg-capacity:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_capacity_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-capacity
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-capacity
.PHONY: test-shared-egg-deferred-journal
test-shared-egg-deferred-journal:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_deferred_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-deferred-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-deferred-journal
.PHONY: test-shared-egg-z8-deferred-journal
test-shared-egg-z8-deferred-journal:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) -DSC64_SHARED_EGG_WIRE_HOST_STAGE -DSC64_SHARED_EGG_JOURNAL_HOST_STAGE tests/sc64_shared_egg_z8_deferred_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-deferred-journal
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z8-deferred-journal

# The opt-in guest-capable integration uses the same real scheduler/journal
# fixtures as the isolated prototype, with the real menu producer enabled.
LATE_EGG_HOST_FLAGS = $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_SHARED_EGG_LATE_SERVICE
.PHONY: test-late-egg-z8-journal test-late-egg-z9-journal test-late-egg-capacity test-late-egg-z8-deferred test-late-egg-z9-deferred
test-late-egg-z8-journal:
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_z8_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-journal
test-late-egg-z9-journal:
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_z9_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-journal
test-late-egg-capacity:
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_capacity_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-capacity
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-capacity
test-late-egg-z8-deferred:
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_z8_deferred_journal_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-deferred
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z8-deferred
test-late-egg-z9-deferred:
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_deferred_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-deferred
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z9-deferred
.PHONY: test-late-egg-z10-journal test-late-egg-custom-journal
test-late-egg-z10-journal:
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) -DSC64_SHARED_EGG_LATE_SERVICE tests/sc64_shared_egg_z10_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-z10-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-z10-journal
test-late-egg-custom-journal:
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-journal
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-journal
# Use HOST-only predecessor fixtures generated by the documented compatibility
# harness; never import these bytes into a guest.
LATE_EGG_PRE_CARGO_DIR ?= /tmp/sc64-late-egg-pre-cargo-z9
LATE_EGG_PRE_CARGO_OLD_ROOT ?= /tmp/sc64-scourge-permissions-base
LATE_EGG_PRE_CARGO_EXTRA = tests/sc64_pre_cargo_compat_shim.c $(filter-out starcraft/game.c,$(STARCRAFT))
.PHONY: test-late-egg-pre-cargo
test-late-egg-pre-cargo:
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
test-late-egg-custom-input:
	$(CLANG) $(LATE_EGG_HOST_FLAGS) tests/sc64_shared_egg_ordinary_input.c tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-input
	$(EGG_TEST_BIN_DIR)/sc64-late-egg-custom-input
