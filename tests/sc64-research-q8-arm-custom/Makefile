# SPDX-License-Identifier: GPL-3.0-or-later
# Explicit experimental ARM roster admission; never included by normal builds.
# make -f Makefile -f tests/sc64-research-q8-arm-custom/Makefile q8-arm-custom-p10
Q8_CUSTOM_ARM_OUT ?= build/q8-arm-custom
Q8_CUSTOM_COVER_DIR ?= .local/hardware-feedback/verified-cover
Q8_CUSTOM_ARM_EXPERIMENT = -DSC64_EXPERIMENTAL_RESEARCH_JOURNAL -DSC64_EXPERIMENTAL_RESEARCH_JOBS -DSC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL -DSC64_RESEARCH_BOUNDED_ARM_PROBE -DSC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT -DSC64_Q8_FLASH_ARM_PROBE -DSC64_Q8_FLASH_ALL_CUSTOM_ARM_PROBE
Q8_CUSTOM_ARM_COMMON = $(COMMON) common/gba_flash1m.c common/gba_flash1m_game_probe.c

define Q8_CUSTOM_ARM_LINK
.PHONY: q8-arm-custom-$(1)
q8-arm-custom-$(1):
	@mkdir -p $$(Q8_CUSTOM_ARM_OUT)
	$$(CC) $$(CFLAGS) -fstack-usage -I$$(Q8_CUSTOM_COVER_DIR) $$(Q8_CUSTOM_ARM_EXPERIMENT) $$($(2)) $$(Q8_CUSTOM_ARM_COMMON) $$(STARCRAFT) $(3) $$(LDFLAGS) -Wl,-Map,$$(Q8_CUSTOM_ARM_OUT)/$(1).map -lgcc -o $$(Q8_CUSTOM_ARM_OUT)/$(1).elf
	$$(SIZE) $$(Q8_CUSTOM_ARM_OUT)/$(1).elf
endef
q8-arm-custom-z7 q8-arm-custom-z9 q8-arm-custom-z10: | $(LATE_ZERG_SIGHT_INPUTS)
$(eval $(call Q8_CUSTOM_ARM_LINK,terran,TERRAN_ROM_FLAGS,))
$(eval $(call Q8_CUSTOM_ARM_LINK,z9,ZERG_Z9_RELEASE_FLAGS,))
$(eval $(call Q8_CUSTOM_ARM_LINK,z10,ZERG_Z10_RELEASE_FLAGS,))
$(eval $(call Q8_CUSTOM_ARM_LINK,z7,ZERG_Z7_RELEASE_FLAGS,))
$(eval $(call Q8_CUSTOM_ARM_LINK,p10,PROTOSS_P10_BASE_FLAGS,$(P7_INTERCEPTOR_FLIGHT_SOURCES)))
$(eval $(call Q8_CUSTOM_ARM_LINK,showcase,SHOWCASE_FLAGS,$(P7_INTERCEPTOR_FLIGHT_SOURCES)))
