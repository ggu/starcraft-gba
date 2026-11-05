include Makefile
EGG_TEST_BIN_DIR ?= /tmp
.PHONY: test-shared-egg-host
SHARED_EGG_HOST_FLAGS = $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) -DSC64_SHARED_EGG_HOST_STAGE
test-shared-egg-host:
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) tests/sc64_shared_egg_host.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-host-fixture
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-host-fixture
	$(CLANG) $(SHARED_EGG_HOST_FLAGS) tests/sc64_z7_egg_production_game.c common/gba.c -o $(EGG_TEST_BIN_DIR)/sc64-shared-egg-z7-legacy-fixture
	$(EGG_TEST_BIN_DIR)/sc64-shared-egg-z7-legacy-fixture
