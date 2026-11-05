# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit late-campaign experiment; normal outputs and flags are unchanged.
SC64_LATE_EGG_PREVIEW_ROOT := build/late-egg-preview
SC64_LATE_EGG_PREVIEW_BUILD := $(SC64_LATE_EGG_PREVIEW_ROOT)/build
SC64_LATE_EGG_PREVIEW_DEPS := $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.ld Makefile mk/sc64-late-egg-preview/Makefile

# Parameters: target, flags, input dependencies, ELF basename, ROM basename,
# title, code, verifier profile.
define SC64_LATE_EGG_PREVIEW_PRODUCT
$$(SC64_LATE_EGG_PREVIEW_BUILD)/$(4).elf: $$(SC64_LATE_EGG_PREVIEW_DEPS) $(3)
	mkdir -p $$(SC64_LATE_EGG_PREVIEW_BUILD)
	$$(CC) $$(CFLAGS) $(2) -DSC64_SHARED_EGG_LATE_SERVICE $$(COMMON) $$(STARCRAFT) $$(LDFLAGS) -Wl,-Map,$$(SC64_LATE_EGG_PREVIEW_BUILD)/$(4).map -lgcc -o $$@
	$$(SIZE) $$@
$$(SC64_LATE_EGG_PREVIEW_BUILD)/$(5).gba: $$(SC64_LATE_EGG_PREVIEW_BUILD)/$(4).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ '$(6)' $(7)
.PHONY: late-egg-preview-$(1)
late-egg-preview-$(1): $$(SC64_LATE_EGG_PREVIEW_BUILD)/$(5).gba
	$$(PYTHON) tools/verify_sc64_late_egg_preview.py --root $$(SC64_LATE_EGG_PREVIEW_ROOT) $(8)
endef
$(eval $(call SC64_LATE_EGG_PREVIEW_PRODUCT,zerg,$(ZERG_Z9_RELEASE_FLAGS),$(ZERG_Z9_CANONICAL_INPUTS),starcraft-zerg-z9,StarCraft-Zerg-Z9-Transport1-GBA,SC ZERG Z9,SCZ9,zerg))
$(eval $(call SC64_LATE_EGG_PREVIEW_PRODUCT,z10,$(ZERG_Z10_RELEASE_FLAGS),$(ZERG_Z10_CANONICAL_INPUTS),starcraft-zerg-z10,StarCraft-Zerg-Z10-Transport1-GBA,SC ZERG Z10,SCZA,z10))
