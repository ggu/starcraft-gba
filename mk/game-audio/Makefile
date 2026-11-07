# Game audio hook checks per profile: audio on and off must match.
define GAME_AUDIO_PROFILE
build/test-game-audio-$(1)-on: tests/game_audio_run.c $$(AUDIO_TEST_DEPS) $(3) | build
	$$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) $$< common/gba.c -o $$@
build/test-game-audio-$(1)-off: tests/game_audio_run.c $$(AUDIO_TEST_DEPS) $(3) | build
	$$(CLANG) -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon $(2) -DGAME_AUDIO_OFF $$< common/gba.c -o $$@
.PHONY: test-game-audio-$(1)
test-game-audio-$(1): build/test-game-audio-$(1)-on build/test-game-audio-$(1)-off
	./build/test-game-audio-$(1)-on hooks
	./build/test-game-audio-$(1)-on > build/test-game-audio-$(1)-on.txt
	./build/test-game-audio-$(1)-off > build/test-game-audio-$(1)-off.txt
	cat build/test-game-audio-$(1)-on.txt
	test "$$$$(grep '^state' build/test-game-audio-$(1)-on.txt)" = "$$$$(grep '^state' build/test-game-audio-$(1)-off.txt)"
	@echo "game audio $(1): audio-on and audio-off state hashes match"
endef
$(eval $(call GAME_AUDIO_PROFILE,default,,))
$(eval $(call GAME_AUDIO_PROFILE,unified,$(UNIFIED_FLAGS),$(RACES_INPUTS)))

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
