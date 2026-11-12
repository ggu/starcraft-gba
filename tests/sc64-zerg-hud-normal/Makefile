# Optional local normal-product A/B qualification. It is not included by the
# release build. Invoke with: make -f Makefile -f tests/sc64-zerg-hud-normal/Makefile TARGET
HUD_NORMAL_OUT ?= build/zerg-hud-normal
.PHONY: hud-normal-z9-off hud-normal-z9-on hud-normal-z10-off hud-normal-z10-on hud-normal-host-z9-off hud-normal-host-z9-on hud-normal-host-z10-off hud-normal-host-z10-on

define HUD_NORMAL_LINK
hud-normal-$(1)-off: ZERG_Z9_HUD_PAGE_FLAGS =
hud-normal-$(1)-off hud-normal-$(1)-on: | $$(LATE_ZERG_SIGHT_INPUTS)
	@mkdir -p $$(HUD_NORMAL_OUT)
	$$(CC) $$(CFLAGS) -fstack-usage -I.local/hardware-feedback/verified-cover $$(ZERG_$(2)_RELEASE_FLAGS) $$(COMMON) $$(STARCRAFT) $$(LDFLAGS) -Wl,-Map,$$(HUD_NORMAL_OUT)/$(1)-$$(@:hud-normal-$(1)-%=%).map -lgcc -o $$(HUD_NORMAL_OUT)/$(1)-$$(@:hud-normal-$(1)-%=%).elf
	$$(SIZE) $$(HUD_NORMAL_OUT)/$(1)-$$(@:hud-normal-$(1)-%=%).elf
hud-normal-host-$(1)-off: ZERG_Z9_HUD_PAGE_FLAGS =
hud-normal-host-$(1)-off hud-normal-host-$(1)-on: | $$(LATE_ZERG_SIGHT_INPUTS)
	@mkdir -p $$(HUD_NORMAL_OUT)
	$$(CLANG) $$(P8_HOST_FLAGS) -I.local/hardware-feedback/verified-cover $$(ZERG_$(2)_CANONICAL_FLAGS) tests/sc64_z3_frame_coverage.c common/gba.c -o $$(HUD_NORMAL_OUT)/host-$(1)-$$(@:hud-normal-host-$(1)-%=%)
	$$(HUD_NORMAL_OUT)/host-$(1)-$$(@:hud-normal-host-$(1)-%=%)
endef
$(eval $(call HUD_NORMAL_LINK,z9,Z9))
$(eval $(call HUD_NORMAL_LINK,z10,Z10))
