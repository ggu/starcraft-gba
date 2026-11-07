# Optional isolated resource/visual probe; never included in release recipes.
# Invoke with: make -f Makefile -f tests/sc64-zerg-hud-ab/Makefile TARGET
HUD_AB_OUTPUT ?= build/zerg-hud-ab
HUD_AB_Q8 = -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_RESEARCH_BOUNDED_ARM_PROBE -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT -DSC64_Q8_FLASH_ARM_PROBE
HUD_AB_EXTRA = common/gba_flash1m.c common/gba_flash1m_game_probe.c
.PHONY: hud-ab-z9-off hud-ab-z9-on hud-ab-z10-off hud-ab-z10-on hud-ab-host-z9-off hud-ab-host-z9-on hud-ab-host-z10-off hud-ab-host-z10-on

define HUD_AB_LINK
hud-ab-$(1)-$(2):
	@mkdir -p $$(HUD_AB_OUTPUT)
	$$(CC) $$(CFLAGS) -fstack-usage -I.local/hardware-feedback/verified-cover $$(HUD_AB_Q8) $$(ZERG_$(3)_RELEASE_FLAGS) $(4) $$(COMMON) $$(STARCRAFT) $$(HUD_AB_EXTRA) $$(LDFLAGS) -Wl,-Map,$$(HUD_AB_OUTPUT)/$(1)-$(2).map -lgcc -o $$(HUD_AB_OUTPUT)/$(1)-$(2).elf
	$$(SIZE) $$(HUD_AB_OUTPUT)/$(1)-$(2).elf
hud-ab-host-$(1)-$(2):
	@mkdir -p $$(HUD_AB_OUTPUT)
	$$(CLANG) $$(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $$(ZERG_$(3)_CANONICAL_FLAGS) $(4) tests/sc64_z3_frame_coverage.c common/gba.c -o $$(HUD_AB_OUTPUT)/host-$(1)-$(2)
	$$(HUD_AB_OUTPUT)/host-$(1)-$(2)
endef
$(eval $(call HUD_AB_LINK,z9,off,Z9,))
$(eval $(call HUD_AB_LINK,z9,on,Z9,-DSC64_HUD_VRAM_BANDS))
$(eval $(call HUD_AB_LINK,z10,off,Z10,))
$(eval $(call HUD_AB_LINK,z10,on,Z10,-DSC64_HUD_VRAM_BANDS))

.PHONY: hud-ab-test-roles
hud-ab-test-roles:
	$(PYTHON) tests/sc64-zerg-hud-ab/test_roles.py
