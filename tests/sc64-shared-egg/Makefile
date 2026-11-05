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
