# SPDX-License-Identifier: GPL-3.0-or-later
# Run from the repository root with -f. Outputs stay in the caller's directory.
include Makefile
RELEASE_EGG_TEST_DIR ?= /tmp/sc64-release-egg-tests
RELEASE_EGG_HOST = -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(SC64_RELEASE_EGG_FLAGS)
RELEASE_EGG_EXTRA = tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
.PHONY: release-egg-host release-egg-terran release-egg-z9 release-egg-z10 release-egg-showcase
release-egg-host: release-egg-terran release-egg-z9 release-egg-z10 release-egg-showcase release-egg-rules
# Each ordinary-input fixture selects a real Hatchery, pays, saves/loads and births.
define RELEASE_EGG_CUSTOM_TEST
release-egg-$(1): $(3)
	mkdir -p $$(RELEASE_EGG_TEST_DIR)
	$$(CLANG) $$(RELEASE_EGG_HOST) $(2) tests/sc64_shared_egg_ordinary_input.c $$(RELEASE_EGG_EXTRA) -o $$(RELEASE_EGG_TEST_DIR)/$(1)-input
	$$(RELEASE_EGG_TEST_DIR)/$(1)-input
endef
$(eval $(call RELEASE_EGG_CUSTOM_TEST,terran,$(TERRAN_ROM_FLAGS),))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,z9,$(ZERG_Z9_RELEASE_FLAGS),))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,z10,$(ZERG_Z10_RELEASE_FLAGS),))
$(eval $(call RELEASE_EGG_CUSTOM_TEST,showcase,$(SHOWCASE_FLAGS),))

# Opt-in campaign Egg service (default off in every shipped product; the
# starting point of the campaign-Egg follow-up). The release selector now maps
# to the custom service only, so these targets select the campaign (late)
# service directly with -DSC64_SHARED_EGG_LATE_SERVICE, as the late preview
# recipes do, and keep the older Z7 custom-vector checks. Run with
# make -f tests/sc64-shared-egg-release/Makefile release-egg-campaign-optin.
RELEASE_EGG_LATE_FLAGS = -DSC64_SHARED_EGG_LATE_SERVICE
.PHONY: release-egg-campaign-optin release-egg-z7 release-egg-z7-legacy release-egg-z9-journal release-egg-z10-journal release-egg-capacity release-egg-deferred
release-egg-campaign-optin: release-egg-z7 release-egg-z7-legacy release-egg-z9-journal release-egg-z10-journal release-egg-capacity release-egg-deferred
$(eval $(call RELEASE_EGG_CUSTOM_TEST,z7,$(ZERG_Z7_RELEASE_FLAGS),))
release-egg-z7-legacy:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(RELEASE_EGG_HOST) $(ZERG_Z7_RELEASE_FLAGS) tests/sc64_shared_egg_custom_z7_legacy.c $(RELEASE_EGG_EXTRA) -o $(RELEASE_EGG_TEST_DIR)/z7-legacy
	$(RELEASE_EGG_TEST_DIR)/z7-legacy
release-egg-z9-journal:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $(RELEASE_EGG_LATE_FLAGS) tests/sc64_shared_egg_z9_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(RELEASE_EGG_TEST_DIR)/z9-journal
	$(RELEASE_EGG_TEST_DIR)/z9-journal
release-egg-z10-journal:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) $(RELEASE_EGG_LATE_FLAGS) tests/sc64_shared_egg_z10_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(RELEASE_EGG_TEST_DIR)/z10-journal
	$(RELEASE_EGG_TEST_DIR)/z10-journal
release-egg-capacity:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $(RELEASE_EGG_LATE_FLAGS) tests/sc64_shared_egg_capacity_game.c common/gba.c -o $(RELEASE_EGG_TEST_DIR)/capacity
	$(RELEASE_EGG_TEST_DIR)/capacity
release-egg-deferred:
	mkdir -p $(RELEASE_EGG_TEST_DIR)
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $(RELEASE_EGG_LATE_FLAGS) tests/sc64_shared_egg_deferred_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(RELEASE_EGG_TEST_DIR)/deferred
	$(RELEASE_EGG_TEST_DIR)/deferred

# Rules in every product: price, larva, supply, timer, paired births, cancel,
# kill, pool and supply caps and a Zerg opponent, then an uninterrupted run
# against a pause-menu save written by one process and loaded by another.
.PHONY: release-egg-rules release-egg-rules-terran release-egg-rules-z9 release-egg-rules-z10 release-egg-rules-showcase
release-egg-rules: release-egg-rules-terran release-egg-rules-z9 release-egg-rules-z10 release-egg-rules-showcase
define RELEASE_EGG_RULES_TEST
release-egg-rules-$(1): $(3) build/generated/weapon-lookup/sc64_weapon_source_indexed.h
	mkdir -p $$(RELEASE_EGG_TEST_DIR)
	$$(CLANG) $$(RELEASE_EGG_HOST) $(2) tests/sc64_shared_egg_rules.c $$(RELEASE_EGG_EXTRA) -o $$(RELEASE_EGG_TEST_DIR)/$(1)-rules
	$$(RELEASE_EGG_TEST_DIR)/$(1)-rules rules
	$$(RELEASE_EGG_TEST_DIR)/$(1)-rules full > $$(RELEASE_EGG_TEST_DIR)/$(1)-full.txt
	$$(RELEASE_EGG_TEST_DIR)/$(1)-rules prefix $$(RELEASE_EGG_TEST_DIR)/$(1).sav
	$$(RELEASE_EGG_TEST_DIR)/$(1)-rules resume $$(RELEASE_EGG_TEST_DIR)/$(1).sav > $$(RELEASE_EGG_TEST_DIR)/$(1)-resume.txt
	diff $$(RELEASE_EGG_TEST_DIR)/$(1)-full.txt $$(RELEASE_EGG_TEST_DIR)/$(1)-resume.txt
	cat $$(RELEASE_EGG_TEST_DIR)/$(1)-resume.txt
endef
$(eval $(call RELEASE_EGG_RULES_TEST,terran,$(TERRAN_ROM_FLAGS),))
$(eval $(call RELEASE_EGG_RULES_TEST,z9,$(ZERG_Z9_RELEASE_FLAGS),$(ZERG_Z9_CANONICAL_INPUTS)))
$(eval $(call RELEASE_EGG_RULES_TEST,z10,$(ZERG_Z10_RELEASE_FLAGS),$(ZERG_Z10_CANONICAL_INPUTS)))
$(eval $(call RELEASE_EGG_RULES_TEST,showcase,$(SHOWCASE_FLAGS),))

# A custom-game save written by an unchanged default-branch build must load in
# the Egg build or be refused cleanly. OLD_TREE is a checkout of that branch with
# its private inputs: make -f tests/sc64-shared-egg-release/Makefile
# release-egg-oldsave OLD_TREE=<path>.
.PHONY: release-egg-oldsave release-egg-oldsave-terran release-egg-oldsave-z9 release-egg-oldsave-z10 release-egg-oldsave-showcase
release-egg-oldsave: release-egg-oldsave-terran release-egg-oldsave-z9 release-egg-oldsave-z10 release-egg-oldsave-showcase
define RELEASE_EGG_OLDSAVE_TEST
release-egg-oldsave-$(1): build/generated/weapon-lookup/sc64_weapon_source_indexed.h
	@test -n "$(OLD_TREE)" || { echo "release-egg-oldsave-$(1): set OLD_TREE to an unchanged default-branch checkout"; exit 1; }
	mkdir -p $$(RELEASE_EGG_TEST_DIR)
	cp tests/sc64_custom_save_cross.c $(OLD_TREE)/tests/
	cd $(OLD_TREE) && $$(CLANG) $$(filter-out $$(SC64_RELEASE_EGG_FLAGS),$$(RELEASE_EGG_HOST)) $(2) tests/sc64_custom_save_cross.c tests/sc64_guardian_journal_shim.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $$(CURDIR)/$$(RELEASE_EGG_TEST_DIR)/$(1)-old-writer
	$$(CLANG) $$(RELEASE_EGG_HOST) $(2) tests/sc64_custom_save_cross.c $$(RELEASE_EGG_EXTRA) -o $$(RELEASE_EGG_TEST_DIR)/$(1)-new-reader
	$$(RELEASE_EGG_TEST_DIR)/$(1)-old-writer write $$(RELEASE_EGG_TEST_DIR)/$(1)-old.sav | tee $$(RELEASE_EGG_TEST_DIR)/$(1)-old-fp.txt
	$$(RELEASE_EGG_TEST_DIR)/$(1)-new-reader read $$(RELEASE_EGG_TEST_DIR)/$(1)-old.sav | tee $$(RELEASE_EGG_TEST_DIR)/$(1)-new-fp.txt
	@if grep -q "LOADED screen=1" $$(RELEASE_EGG_TEST_DIR)/$(1)-new-fp.txt; then \
	  grep "^FP" $$(RELEASE_EGG_TEST_DIR)/$(1)-old-fp.txt > $$(RELEASE_EGG_TEST_DIR)/$(1)-a.txt; \
	  grep "^FP" $$(RELEASE_EGG_TEST_DIR)/$(1)-new-fp.txt > $$(RELEASE_EGG_TEST_DIR)/$(1)-b.txt; \
	  diff $$(RELEASE_EGG_TEST_DIR)/$(1)-a.txt $$(RELEASE_EGG_TEST_DIR)/$(1)-b.txt && echo "old save LOADS with matching state"; \
	else echo "old save REJECTED cleanly (stayed on title)"; fi
endef
$(eval $(call RELEASE_EGG_OLDSAVE_TEST,terran,$(TERRAN_ROM_FLAGS)))
$(eval $(call RELEASE_EGG_OLDSAVE_TEST,z9,$(ZERG_Z9_RELEASE_FLAGS)))
$(eval $(call RELEASE_EGG_OLDSAVE_TEST,z10,$(ZERG_Z10_RELEASE_FLAGS)))
$(eval $(call RELEASE_EGG_OLDSAVE_TEST,showcase,$(SHOWCASE_FLAGS)))
