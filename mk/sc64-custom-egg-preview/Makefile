# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit opt-in products. Standard profile outputs and flags remain unchanged.
SC64_EGG_PREVIEW_ROOT := build/custom-egg-preview
SC64_EGG_PREVIEW_BUILD := $(SC64_EGG_PREVIEW_ROOT)/build
SC64_EGG_PREVIEW_FACT_DIR := $(SC64_EGG_PREVIEW_ROOT)/p10-facts
SC64_EGG_PREVIEW_FACT := $(SC64_EGG_PREVIEW_FACT_DIR)/sc64_fact_tables_generated.h
SC64_EGG_PREVIEW_P10_FLAGS = $(patsubst -I$(P10_FACT_V2_DIR),-I$(SC64_EGG_PREVIEW_FACT_DIR),$(PROTOSS_P10_BASE_FLAGS))
SC64_EGG_PREVIEW_DEPS := $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.ld Makefile mk/sc64-custom-egg-preview/Makefile

$(SC64_EGG_PREVIEW_FACT): $(P8_AVAILABILITY_HEADER) tools/sc64_fact_tables_gen.c $(wildcard starcraft/*.h) $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile mk/sc64-custom-egg-preview/Makefile
	mkdir -p $(SC64_EGG_PREVIEW_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -DSC64_SHARED_EGG_WIRE_HOST_STAGE $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o $(SC64_EGG_PREVIEW_FACT_DIR)/generator
	$(SC64_EGG_PREVIEW_FACT_DIR)/generator > $@.tmp
	mv $@.tmp $@

# Parameters: target name, product flags, extra C sources, ELF basename,
# ROM basename, title, code, product inputs, verifier profile.
define SC64_EGG_PREVIEW_PRODUCT
$$(SC64_EGG_PREVIEW_BUILD)/$(4).elf: $$(SC64_EGG_PREVIEW_DEPS) $(8)
	mkdir -p $$(SC64_EGG_PREVIEW_BUILD)
	$$(CC) $$(CFLAGS) $(2) -DSC64_SHARED_EGG_CUSTOM_SERVICE $$(COMMON) $$(STARCRAFT) $(3) $$(LDFLAGS) -Wl,-Map,$$(SC64_EGG_PREVIEW_BUILD)/$(4).map -lgcc -o $$@
	$$(SIZE) $$@
$$(SC64_EGG_PREVIEW_BUILD)/$(5).gba: $$(SC64_EGG_PREVIEW_BUILD)/$(4).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ '$(6)' $(7)
.PHONY: custom-egg-preview-$(1)
custom-egg-preview-$(1): $$(SC64_EGG_PREVIEW_BUILD)/$(5).gba
	$$(PYTHON) tools/verify_sc64_egg_preview.py --root $$(SC64_EGG_PREVIEW_ROOT) $(9)
endef
$(eval $(call SC64_EGG_PREVIEW_PRODUCT,terran,$(TERRAN_ROM_FLAGS),,starcraft-terran,StarCraft-Terran-Transport1-GBA,SC TERRAN 10,SCT8,$(TERRAN_FULL_INPUTS) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) $(BC_INPUTS),terran))
$(eval $(call SC64_EGG_PREVIEW_PRODUCT,zerg,$(ZERG_Z9_RELEASE_FLAGS),,starcraft-zerg-z9,StarCraft-Zerg-Z9-Transport1-GBA,SC ZERG Z9,SCZ9,$(ZERG_Z9_CANONICAL_INPUTS),zerg))
$(eval $(call SC64_EGG_PREVIEW_PRODUCT,z7,$(ZERG_Z7_RELEASE_FLAGS),,starcraft-zerg-z7,StarCraft-Zerg-Z7-Transport1-GBA,SC ZERG Z7,SCZ7,$(ZERG_Z7_CANONICAL_INPUTS),z7))
$(eval $(call SC64_EGG_PREVIEW_PRODUCT,protoss,$(SC64_EGG_PREVIEW_P10_FLAGS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),starcraft-protoss-p10,StarCraft-Protoss-P10-Transport1-GBA,SC PROTOSS10,SCPA,$(filter-out $(P10_FACT_V2_HEADER),$(PROTOSS_P10_INPUTS)) $(SC64_EGG_PREVIEW_FACT),protoss))
$(eval $(call SC64_EGG_PREVIEW_PRODUCT,showcase,$(SHOWCASE_FLAGS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),starcraft-showcase,StarCraft-Showcase-Transport1-GBA,SC SHOWCASE,SCSH,$(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(COMMAND_ICON_HEADER) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) $(BC_INPUTS),showcase))
