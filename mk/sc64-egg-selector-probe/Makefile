# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit qualification products; no normal profile recipe is modified.
include tests/sc64-shared-egg/Makefile
EGG_SELECTOR_ROOT ?= build/egg-selector-probe
EGG_SELECTOR_FACT_DIR = $(EGG_SELECTOR_ROOT)/p10-facts
EGG_SELECTOR_P10_ON_FLAGS = $(patsubst -I$(P10_FACT_V2_DIR),-I$(EGG_SELECTOR_FACT_DIR),$(PROTOSS_P10_FLAGS))
.PHONY: egg-selector-facts
egg-selector-facts:
	mkdir -p $(EGG_SELECTOR_FACT_DIR)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) -Istarcraft -DSC64_SHARED_EGG_WIRE_HOST_STAGE $(filter-out -DSC64_FACT_TABLES,$(patsubst -I$(P7_FACT_V2_DIR),-I$(P10_FACT_V2_DIR),$(P7_FACT_V2_FLAGS)) $(P8_CAMPAIGN_DEFINES) $(P9_CAMPAIGN_DEFINES) $(P10_CAMPAIGN_DEFINES)) $(SC64_RELEASE_TRANSPORT_FLAGS) -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT -DSC64_BATTLECRUISER_YAMATO_EXPERIMENT tools/sc64_fact_tables_gen.c -o $(EGG_SELECTOR_FACT_DIR)/generator
	$(EGG_SELECTOR_FACT_DIR)/generator > $(EGG_SELECTOR_FACT_DIR)/sc64_fact_tables_generated.h.tmp
	mv $(EGG_SELECTOR_FACT_DIR)/sc64_fact_tables_generated.h.tmp $(EGG_SELECTOR_FACT_DIR)/sc64_fact_tables_generated.h

# Parameters: profile, mode, flags, extra sources, ELF, ROM, title, code, verifier.
define EGG_SELECTOR_PRODUCT
.PHONY: egg-selector-$(1)-$(2)
egg-selector-$(1)-$(2):
	mkdir -p $$(EGG_SELECTOR_ROOT)/$(2)/$(1)/build
	$$(CC) $$(CFLAGS) -I.local/hardware-feedback/verified-cover $(3) $$(if $$(filter on,$(2)),-DSC64_SHARED_EGG_SERVICE) $$(COMMON) $$(STARCRAFT) $(4) $$(LDFLAGS) -Wl,-Map,$$(EGG_SELECTOR_ROOT)/$(2)/$(1)/build/$(5).map -lgcc -o $$(EGG_SELECTOR_ROOT)/$(2)/$(1)/build/$(5).elf
	$$(OBJCOPY) -O binary $$(EGG_SELECTOR_ROOT)/$(2)/$(1)/build/$(5).elf $$(EGG_SELECTOR_ROOT)/$(2)/$(1)/build/$(6).gba
	$$(PYTHON) tools/fix_rom.py $$(EGG_SELECTOR_ROOT)/$(2)/$(1)/build/$(6).gba '$(7)' $(8)
	$$(PYTHON) $(9) --root $$(EGG_SELECTOR_ROOT)/$(2)/$(1) $(10)
endef
$(foreach mode,off on,$(eval $(call EGG_SELECTOR_PRODUCT,terran,$(mode),$(TERRAN_ROM_FLAGS),,starcraft-terran,StarCraft-Terran-Transport1-GBA,SC TERRAN 10,SCT8,tools/verify_sc64_egg_preview.py,terran)))
$(foreach mode,off on,$(eval $(call EGG_SELECTOR_PRODUCT,z7,$(mode),$(ZERG_Z7_RELEASE_FLAGS),,starcraft-zerg-z7,StarCraft-Zerg-Z7-Transport1-GBA,SC ZERG Z7,SCZ7,tools/verify_sc64_egg_preview.py,z7)))
$(foreach mode,off on,$(eval $(call EGG_SELECTOR_PRODUCT,z9,$(mode),$(ZERG_Z9_RELEASE_FLAGS),,starcraft-zerg-z9,StarCraft-Zerg-Z9-Transport1-GBA,SC ZERG Z9,SCZ9,tools/verify_sc64_late_egg_preview.py,zerg)))
$(foreach mode,off on,$(eval $(call EGG_SELECTOR_PRODUCT,z10,$(mode),$(ZERG_Z10_RELEASE_FLAGS),,starcraft-zerg-z10,StarCraft-Zerg-Z10-Transport1-GBA,SC ZERG Z10,SCZA,tools/verify_sc64_late_egg_preview.py,z10)))
$(foreach mode,off on,$(eval $(call EGG_SELECTOR_PRODUCT,p10,$(mode),$(if $(filter on,$(mode)),$(EGG_SELECTOR_P10_ON_FLAGS),$(PROTOSS_P10_FLAGS)),$(P7_INTERCEPTOR_FLIGHT_SOURCES),starcraft-protoss-p10,StarCraft-Protoss-P10-Transport1-GBA,SC PROTOSS10,SCPA,tools/verify_sc64_egg_preview.py,protoss)))
$(foreach mode,off on,$(eval $(call EGG_SELECTOR_PRODUCT,showcase,$(mode),$(SHOWCASE_FLAGS),$(P7_INTERCEPTOR_FLIGHT_SOURCES),starcraft-showcase,StarCraft-Showcase-Transport1-GBA,SC SHOWCASE,SCSH,tools/verify_sc64_egg_preview.py,showcase)))

.PHONY: egg-selector-contract egg-selector-z6-host
egg-selector-contract:
	$(CLANG) -std=c11 -fsyntax-only -DEXPECT_OFF tests/sc64_egg_selector_contract.c
	$(CLANG) -std=c11 -fsyntax-only -DEXPECT_CUSTOM -DSC64_SHARED_EGG_SERVICE tests/sc64_egg_selector_contract.c
	$(CLANG) -std=c11 -fsyntax-only -DEXPECT_LATE -DSC64_SHARED_EGG_SERVICE -DSC64_Z9_RUNTIME tests/sc64_egg_selector_contract.c
egg-selector-z6-host:
	mkdir -p $(EGG_SELECTOR_ROOT)/host
	$(CLANG) $(filter-out -DSC64_SHARED_EGG_LATE_SERVICE,$(Z6_EGG_HOST_FLAGS)) -DSC64_SHARED_EGG_SERVICE tests/sc64_shared_egg_z6_journal_game.c tests/sc64_shared_egg_journal_shim.c -o $(EGG_SELECTOR_ROOT)/host/z6-selector
	$(EGG_SELECTOR_ROOT)/host/z6-selector
.PHONY: egg-selector-z7-host
egg-selector-z7-host:
	mkdir -p $(EGG_SELECTOR_ROOT)/host
	$(CLANG) -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -I.local/hardware-feedback/verified-cover $(ZERG_Z7_RELEASE_FLAGS) -DSC64_SHARED_EGG_SERVICE tests/sc64_shared_egg_custom_z7_legacy.c tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(EGG_SELECTOR_ROOT)/host/z7-selector
	$(EGG_SELECTOR_ROOT)/host/z7-selector
.PHONY: egg-selector-custom-input
egg-selector-custom-input:
	mkdir -p $(EGG_SELECTOR_ROOT)/host
	$(CLANG) -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -I.local/hardware-feedback/verified-cover $(ZERG_Z7_RELEASE_FLAGS) -DSC64_SHARED_EGG_SERVICE tests/sc64_egg_custom_input.c tests/sc64_egg_guest_save_bridge.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(EGG_SELECTOR_ROOT)/host/custom-input
	$(EGG_SELECTOR_ROOT)/host/custom-input $(EGG_SELECTOR_ROOT)/host/custom-input.json $(EGG_SELECTOR_ROOT)/host/custom-host-only.sav
.PHONY: egg-selector-guest-save-reader
egg-selector-guest-save-reader:
	mkdir -p $(EGG_SELECTOR_ROOT)/host
	$(CLANG) -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -I.local/hardware-feedback/verified-cover $(ZERG_Z7_RELEASE_FLAGS) -DSC64_SHARED_EGG_SERVICE tests/sc64_egg_guest_save_reader.c tests/sc64_egg_guest_save_bridge.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(EGG_SELECTOR_ROOT)/host/guest-save-reader
