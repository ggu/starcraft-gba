# SPDX-License-Identifier: GPL-3.0-or-later
# The controller is also included directly in several prepared HOST fixtures.
SKIRMISH_CONTROLLER_PROFILES := terran zerg protoss showcase
SKIRMISH_CONTROLLER_DEPS := tests/sc64_skirmish_campaign_run.c tests/sc64_visible_mineral_cursor.h
$(addprefix build/test-sc64-skirmish-match-,$(SKIRMISH_CONTROLLER_PROFILES) zerg-z7) \
$(addprefix build/test-sc64-skirmish-build-controller-,$(SKIRMISH_CONTROLLER_PROFILES)) \
$(addprefix build/test-sc64-skirmish-buttons-,$(SKIRMISH_CONTROLLER_PROFILES)) \
$(addprefix build/test-sc64-depot-approach-,$(DEPOT_APPROACH_CASES)) \
$(addprefix build/test-native-melee-start-,terran z10 p10 showcase) \
build/test-sc64-p7-campaign-skirmish build/test-sc64-protoss-skirmish-pylons: $(SKIRMISH_CONTROLLER_DEPS)
