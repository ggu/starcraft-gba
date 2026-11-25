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

# The interrupt's two hand-written routines against their C references, on
# the ARM7TDMI and over the whole private bank (tests/audio_codec_arm_probe.c).
# Run the ROM in mGBA with -l 7 (its decoder sum must equal the one
# `make test-audio-bank` prints), or use check-audio-arm-probe below.
build/audio-arm-probe.elf: tests/audio_codec_arm_probe.c common/audio_bank.S common/audio_codec.h common/audio_mixer.h common/gba.h common/start.s common/gba.ld $(SC64_AUDIO_DIR)/sc64_audio_generated.h $(SC64_AUDIO_DIR)/sc64_audio_generated.bin $(SC64_AUDIO_BANK_CONFIG) | build
	$(CC) $(CFLAGS) -DSC64_AUDIO_IMPORTED -I$(SC64_AUDIO_DIR) '-DSC64_AUDIO_BLOB="$(SC64_AUDIO_DIR)/sc64_audio_generated.bin"' common/start.s common/audio_bank.S $< $(LDFLAGS) -Wl,-Map,build/audio-arm-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Audio-Probe-GBA.gba: build/audio-arm-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC AUDIO PRB' SCDP
.PHONY: audio-arm-probe check-audio-arm-probe
audio-arm-probe: build/StarCraft-Audio-Probe-GBA.gba
# The same in the headless mGBA runner, judged against the host decode:
# make check-audio-arm-probe AUDIO_ARM_RUNNER=<sc64-headless-replay>
check-audio-arm-probe: build/StarCraft-Audio-Probe-GBA.gba build/test-audio-bank tools/sc64_audio_arm_probe.py
	@if [ -z "$(AUDIO_ARM_RUNNER)" ]; then echo "set AUDIO_ARM_RUNNER to the headless mGBA runner"; exit 2; fi
	./build/test-audio-bank $(SC64_AUDIO_DIR)/sc64_audio_generated.bin $(SC64_AUDIO_DIR)/sc64_audio_manifest.json > build/audio-arm-probe-host.txt
	$(PYTHON) tools/sc64_audio_arm_probe.py build/StarCraft-Audio-Probe-GBA.gba build/audio-arm-probe.elf $(AUDIO_ARM_RUNNER) build/audio-arm-probe-host.txt

# Replays a request timeline through the mixer with the private bank and
# writes the bytes the interrupt leaves in the two DMA buffers
# (tests/audio_bank_timeline.c).
build/test-audio-bank-timeline: tests/audio_bank_timeline.c common/audio_mixer.h common/audio_codec.h common/gba.h $(SC64_AUDIO_DIR)/sc64_audio_generated.h $(SC64_AUDIO_BANK_CONFIG) | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_AUDIO_IMPORTED -I$(SC64_AUDIO_DIR) $< -o $@

# Plays the whole private bank through the mixer against a plain model
# (tests/audio_bank_mix.c); part of test-audio-bank.
build/test-audio-bank-mix: tests/audio_bank_mix.c common/audio_mixer.h common/audio_codec.h common/gba.h $(SC64_AUDIO_DIR)/sc64_audio_generated.h $(SC64_AUDIO_BANK_CONFIG) | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon -DSC64_AUDIO_IMPORTED -I$(SC64_AUDIO_DIR) $< -o $@
.PHONY: test-audio-bank-mix
test-audio-bank-mix: build/test-audio-bank-mix
	./build/test-audio-bank-mix $(SC64_AUDIO_DIR)/sc64_audio_generated.bin
test-audio-bank: test-audio-bank-mix
