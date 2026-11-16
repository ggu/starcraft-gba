# SPDX-License-Identifier: GPL-3.0-or-later
# Opt in: make -f Makefile -f mk/carrier-release/Makefile carrier-release
include tests/sc64-terran-carrier-runtime/Makefile
include tests/sc64-zerg-carrier-runtime/Makefile

SC64_CARRIER_RELEASE_PROFILE ?= all
SC64_CARRIER_RELEASE_PRODUCTS := terran zerg-z10 showcase protoss-p10

SC64_CARRIER_RELEASE_ROM_terran := build/StarCraft-Terran-Carrier-Probe.gba
SC64_CARRIER_RELEASE_ROM_zerg-z10 := build/StarCraft-Zerg-Z10-Carrier-Probe.gba
SC64_CARRIER_RELEASE_ROM_showcase := build/StarCraft-Showcase-Carrier-Probe-GBA.gba
SC64_CARRIER_RELEASE_ROM_protoss-p10 := build/StarCraft-Protoss-P10-Transport1-GBA.gba

SC64_CARRIER_RELEASE_TEST_terran := test-sc64-terran-carrier-runtime
SC64_CARRIER_RELEASE_TEST_zerg-z10 := test-sc64-zerg-carrier-runtime
SC64_CARRIER_RELEASE_TEST_showcase := test-sc64-showcase-carrier
SC64_CARRIER_RELEASE_TEST_protoss-p10 := test-sc64-p10-gantrithor

ifeq ($(SC64_CARRIER_RELEASE_PROFILE),all)
SC64_CARRIER_RELEASE_SELECTED := $(SC64_CARRIER_RELEASE_PRODUCTS)
else ifneq ($(filter $(SC64_CARRIER_RELEASE_PROFILE),$(SC64_CARRIER_RELEASE_PRODUCTS)),$(SC64_CARRIER_RELEASE_PROFILE))
$(error Unknown Carrier release profile '$(SC64_CARRIER_RELEASE_PROFILE)')
else ifneq ($(words $(SC64_CARRIER_RELEASE_PROFILE)),1)
$(error Choose one Carrier release profile or 'all')
else
SC64_CARRIER_RELEASE_SELECTED := $(SC64_CARRIER_RELEASE_PROFILE)
endif

.PHONY: carrier-release test-carrier-release carrier-release-plan
carrier-release: $(foreach profile,$(SC64_CARRIER_RELEASE_SELECTED),$(SC64_CARRIER_RELEASE_ROM_$(profile)))
test-carrier-release: $(foreach profile,$(SC64_CARRIER_RELEASE_SELECTED),$(SC64_CARRIER_RELEASE_TEST_$(profile)))
carrier-release-plan:
	@$(foreach profile,$(SC64_CARRIER_RELEASE_SELECTED),printf '%s\n' '$(profile) $(SC64_CARRIER_RELEASE_ROM_$(profile)) $(SC64_CARRIER_RELEASE_TEST_$(profile))';)
