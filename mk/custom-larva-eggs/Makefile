# SPDX-License-Identifier: GPL-3.0-or-later
# Custom Game Larva and Eggs on the real engine (tests/sc64_custom_larva_eggs.c),
# built with the flag vector of each product that has Custom Games. The four
# delivered products use the flags of their ROM rule.
#
# The Zerg profiles also build the same test with the capability off. That
# binary writes the serial-queue record an earlier build writes, and must
# refuse a record that holds Eggs. The other products append the Egg to their
# unit roster, which their generated fact tables are built for, so they cannot
# be built without the capability here; their earlier record is placed by the
# test itself (old-fixture).
# Prepared private inputs are required; nothing here enters ./dev test.
CLE_DIR := build/custom-larva-eggs
CLE_HOST = $(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable
CLE_SOURCES = tests/sc64_custom_larva_eggs.c tests/sc64_yamato_host_sram_shim.c $(filter-out starcraft/game.c,$(STARCRAFT))
CLE_DEPS = tests/sc64_custom_larva_eggs.c tests/sc64_skirmish_campaign_run.c tests/sc64_visible_mineral_cursor.h \
	tests/sc64_yamato_host_sram_shim.c $(COMMON) $(STARCRAFT) $(wildcard starcraft/*.h) $(COMMON_HEADERS) \
	mk/custom-larva-eggs/Makefile
CLE_TERRAN_FLAGS = $(TERRAN_CARRIER_FLAGS) $(TERRAN_ROM_FLAGS)
CLE_Z10_PLAIN_FLAGS = $(Z10_CARRIER_FLAGS) $(Z10_PADDED_IMPACT_FLAGS) $(ZERG_Z10_RELEASE_FLAGS)
CLE_Z10_FLAGS = $(Z10_FACT_FLAGS) $(CLE_Z10_PLAIN_FLAGS)
CLE_P10_FLAGS = $(P10_PADDED_IMPACT_FLAGS) $(PROTOSS_P10_FLAGS)
CLE_TERRAN_INPUTS = $(TERRAN_FULL_INPUTS) $(SKIRMISH_CONTROLLER_EXTRA_INPUTS) $(BC_INPUTS)
CLE_Z10_INPUTS = $(ZERG_Z10_CANONICAL_INPUTS) $(Z10_FACT_HEADER)
CLE_PROFILES := terran zerg-z10 protoss-p10 showcase zerg-z7 protoss-p7
# The earlier producer: the capability off, and with it the production-build
# contract that requires the capability (sc64_engine_capabilities.h).
CLE_QUEUE_FLAGS := -USC64_CUSTOM_LARVA_EGGS -USC64_ENGINE_CORE_REQUIRED

# The checks every profile runs. The run's SRAM images and records stay in
# the profile's .run folder.
define CLE_RUN
	mkdir -p $$(CLE_DIR)/$(1).run
	./$$(CLE_DIR)/$(1) lifecycle
	./$$(CLE_DIR)/$(1) save $$(CLE_DIR)/$(1).run/eggs.sav > $$(CLE_DIR)/$(1).run/saved
	./$$(CLE_DIR)/$(1) resume $$(CLE_DIR)/$(1).run/eggs.sav > $$(CLE_DIR)/$(1).run/resumed
	$$(PYTHON) tools/sc64_custom_larva_eggs_resume.py $$(CLE_DIR)/$(1).run/saved $$(CLE_DIR)/$(1).run/resumed
	./$$(CLE_DIR)/$(1) save $$(CLE_DIR)/$(1).run/eggs3.sav 1 3 > $$(CLE_DIR)/$(1).run/saved3
	./$$(CLE_DIR)/$(1) resume $$(CLE_DIR)/$(1).run/eggs3.sav > $$(CLE_DIR)/$(1).run/resumed3
	$$(PYTHON) tools/sc64_custom_larva_eggs_resume.py $$(CLE_DIR)/$(1).run/saved3 $$(CLE_DIR)/$(1).run/resumed3
endef

# Parameters: profile, flags variable, extra translation units variable,
# generated inputs variable.
define CLE_PROFILE
$$(CLE_DIR)/$(1): $$(CLE_DEPS) $$($(4)) | build
	mkdir -p $$(CLE_DIR)
	$$(CLE_HOST) $$($(2)) $$(CLE_SOURCES) $$($(3)) -o $$@
.PHONY: test-custom-larva-eggs-$(1)
test-custom-larva-eggs-$(1): $$(CLE_DIR)/$(1)
$(call CLE_RUN,$(1))
	./$$(CLE_DIR)/$(1) old-fixture $$(CLE_DIR)/$(1).run/queue.sav
	./$$(CLE_DIR)/$(1) old-read $$(CLE_DIR)/$(1).run/queue.sav
endef

# The same, with the capability-off writer and reader for the earlier record.
# Fifth parameter: the flags variable of that build. A product's fact tables
# record the capability they were generated with, so that build leaves them
# out; they are a speed table and no part of a record.
define CLE_ZERG_PROFILE
$$(CLE_DIR)/$(1): $$(CLE_DEPS) $$($(4)) | build
	mkdir -p $$(CLE_DIR)
	$$(CLE_HOST) $$($(2)) $$(CLE_SOURCES) $$($(3)) -o $$@
$$(CLE_DIR)/$(1)-queue: $$(CLE_DEPS) $$($(4)) | build
	mkdir -p $$(CLE_DIR)
	$$(CLE_HOST) $$($(5)) $$(CLE_QUEUE_FLAGS) $$(CLE_SOURCES) $$($(3)) -o $$@
.PHONY: test-custom-larva-eggs-$(1)
test-custom-larva-eggs-$(1): $$(CLE_DIR)/$(1) $$(CLE_DIR)/$(1)-queue
$(call CLE_RUN,$(1))
	./$$(CLE_DIR)/$(1)-queue old-write $$(CLE_DIR)/$(1).run/queue.sav
	./$$(CLE_DIR)/$(1) old-read $$(CLE_DIR)/$(1).run/queue.sav
	./$$(CLE_DIR)/$(1)-queue old-reject $$(CLE_DIR)/$(1).run/eggs.sav
endef

$(eval $(call CLE_PROFILE,terran,CLE_TERRAN_FLAGS,P7_INTERCEPTOR_FLIGHT_SOURCES,CLE_TERRAN_INPUTS))
$(eval $(call CLE_ZERG_PROFILE,zerg-z10,CLE_Z10_FLAGS,P7_INTERCEPTOR_FLIGHT_SOURCES,CLE_Z10_INPUTS,CLE_Z10_PLAIN_FLAGS))
$(eval $(call CLE_PROFILE,protoss-p10,CLE_P10_FLAGS,P7_INTERCEPTOR_FLIGHT_SOURCES,PROTOSS_P10_INPUTS))
$(eval $(call CLE_PROFILE,showcase,SHOWCASE_FLAGS,P7_INTERCEPTOR_FLIGHT_SOURCES,SKIRMISH_AI_SHOWCASE_INPUTS))
$(eval $(call CLE_ZERG_PROFILE,zerg-z7,CANONICAL_ZERG_PROFILE_FLAGS,,CANONICAL_ZERG_PROFILE_INPUTS,CANONICAL_ZERG_PROFILE_FLAGS))
$(eval $(call CLE_PROFILE,protoss-p7,PROTOSS_ROM_FLAGS,P7_INTERCEPTOR_FLIGHT_SOURCES,PROTOSS_P7_CANONICAL_INPUTS))

.PHONY: test-custom-larva-eggs
test-custom-larva-eggs: $(addprefix test-custom-larva-eggs-,$(CLE_PROFILES))
