# Advisor alert checks at a race ROM's flags. Need the private bank and inputs.
define ALERT_ARM_PROBE
build/starcraft-alert-probe-$(1).elf: tests/game_audio_arm_probe.c $$(COMMON_HEADERS) $$(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.ld $(3) | build
	$$(CC) $$(CFLAGS) $(2) -Wl,--wrap=audio_play_sample common/start.s common/gba.c common/memory.c common/audio_bank.S $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(LDFLAGS) -Wl,-Map,build/starcraft-alert-probe-$(1).map -lgcc -o $$@
	$$(SIZE) $$@
build/StarCraft-Alert-Probe-$(1)-GBA.gba: build/starcraft-alert-probe-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC ALERT PRB' SCAP
.PHONY: alert-arm-probe-$(1)
alert-arm-probe-$(1): build/StarCraft-Alert-Probe-$(1)-GBA.gba
endef
$(eval $(call ALERT_ARM_PROBE,terran,$(TERRAN_ROM_FLAGS),$(TERRAN_FULL_INPUTS)))
