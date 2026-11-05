# SPDX-License-Identifier: GPL-3.0-or-later
# Run from the repository root with -f. Outputs stay in the caller's directory.
include Makefile
RELEASE_EGG_TEST_DIR ?= /tmp/sc64-release-egg-tests
RELEASE_EGG_HOST = -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(SC64_RELEASE_EGG_FLAGS)
RELEASE_EGG_EXTRA = tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
.PHONY: release-egg-host release-egg-terran release-egg-z7 release-egg-z9 release-egg-z10 release-egg-p10 release-egg-showcase
release-egg-host: release-egg-terran release-egg-z7 release-egg-z9 release-egg-z10 release-egg-p10 release-egg-showcase
# Each ordinary-input fixture selects a real Hatchery, pays, saves/loads and births.
define RELEASE_EGG_CUSTOM_TEST
release-egg-$(1): $(3)
	mkdir -p $$(RELEASE_EGG_TEST_DIR)
	$$(CLANG) $$(RELEASE_EGG_HOST) $(2) tests/sc64_shared_egg_ordinary_input.c $$(RELEASE_EGG_EXTRA) -o $$(RELEASE_EGG_TEST_DIR)/$(1)-input
	$$(RELEASE_EGG_TEST_DIR)/$(1)-input
endef
$(eval $(call RELEASE_EGG_CUSTOM_TEST,terran,$(TERRAN_ROM_FLAGS),))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,z7,$(ZERG_Z7_RELEASE_FLAGS),))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,z9,$(ZERG_Z9_RELEASE_FLAGS),))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,z10,$(ZERG_Z10_RELEASE_FLAGS),))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,p10,$(filter-out $(SC64_RELEASE_EGG_FLAGS),$(P10_EGG_FLAGS)),$(P10_EGG_FACT_HEADER)))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,showcase,$(SHOWCASE_FLAGS),))
.PHONY: release-egg-z7-legacy release-egg-z9-journal release-egg-z10-journal release-egg-capacity release-egg-deferred
release-egg-z7-legacy:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(RELEASE_EGG_HOST) $(ZERG_Z7_RELEASE_FLAGS) tests/sc64_shared_egg_custom_z7_legacy.c $(RELEASE_EGG_EXTRA) -o $(RELEASE_EGG_TEST_DIR)/z7-legacy
	$(RELEASE_EGG_TEST_DIR)/z7-legacy
release-egg-z9-journal:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $(SC64_RELEASE_EGG_FLAGS) tests/sc64_shared_egg_z9_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(RELEASE_EGG_TEST_DIR)/z9-journal
	$(RELEASE_EGG_TEST_DIR)/z9-journal
release-egg-z10-journal:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) $(SC64_RELEASE_EGG_FLAGS) tests/sc64_shared_egg_z10_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(RELEASE_EGG_TEST_DIR)/z10-journal
	$(RELEASE_EGG_TEST_DIR)/z10-journal
release-egg-capacity:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $(SC64_RELEASE_EGG_FLAGS) tests/sc64_shared_egg_capacity_game.c common/gba.c -o $(RELEASE_EGG_TEST_DIR)/capacity
	$(RELEASE_EGG_TEST_DIR)/capacity
release-egg-deferred:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $(SC64_RELEASE_EGG_FLAGS) tests/sc64_shared_egg_deferred_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(RELEASE_EGG_TEST_DIR)/deferred
	$(RELEASE_EGG_TEST_DIR)/deferred
