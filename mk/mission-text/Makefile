# Modal in-mission text boxes: the private bank, the asset-free model test and
# the no-debugger ARM probes that show a box on each campaign product's layout.
.PHONY: sc64-mission-text test-sc64-mission-text
sc64-mission-text:
	python3 tools/sc64_mission_text_pack.py
.local/sc64/mission-text/generated/sc64_mission_text_generated.h:
	@echo 'Private mission text is missing. Run make sc64-mission-text with the prepared local ROM and reference checkout.'
	@false
build/test-sc64-mission-text: tests/sc64_mission_text.c starcraft/sc64_mission_text.h common/gba.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft $< -o $@
test-sc64-mission-text: build/test-sc64-mission-text
	./build/test-sc64-mission-text
test-starcraft: test-sc64-mission-text

# tests/sc64_mission_text_arm_probe.c replaces common/main.c. Each ROM holds a
# product's exact flag vector plus the missions to open; run one through
# tools/sc64_mission_text_arm_probe.py.
MT_COMMA := ,
MT_PROBE_COMMON := $(filter-out common/main.c,$(COMMON))
define MT_PROBE
build/starcraft-mt-probe-$(1).elf: tests/sc64_mission_text_arm_probe.c $$(MT_PROBE_COMMON) $$(COMMON_HEADERS) $$(STARCRAFT) $(5) $$(wildcard starcraft/*.h) $$($(3)) common/gba.ld Makefile mk/mission-text/Makefile | build
	$$(CC) $$(CFLAGS) $$($(2)) -DPROBE_MISSIONS=$(subst /,$$(MT_COMMA),$(4)) $$(MT_PROBE_COMMON) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $(5) $$(LDFLAGS) -Wl,-Map,build/starcraft-mt-probe-$(1).map -lgcc -o $$@
build/StarCraft-MissionText-Probe-$(1).gba: build/starcraft-mt-probe-$(1).elf tools/fix_rom.py
	$$(OBJCOPY) -O binary $$< $$@
	$$(PYTHON) tools/fix_rom.py $$@ 'SC MT PROBE' SCMT
MT_PROBE_ROMS += build/StarCraft-MissionText-Probe-$(1).gba
endef
$(eval $(call MT_PROBE,terran,TERRAN_ROM_FLAGS,TERRAN_FULL_INPUTS,4/5/6/7/8/9/10/11/12/13))
$(eval $(call MT_PROBE,zerg,ZERG_Z7_RELEASE_FLAGS,ZERG_Z7_CANONICAL_INPUTS,0/1/14/15/16/17))
$(eval $(call MT_PROBE,protoss,PROTOSS_P7_CANONICAL_FLAGS,PROTOSS_P7_CANONICAL_INPUTS,2/3/22/23/24/25/26,$$(P7_INTERCEPTOR_FLIGHT_SOURCES)))
$(eval $(call MT_PROBE,p10,PROTOSS_P10_FLAGS,PROTOSS_P10_INPUTS,27/28/29,$$(P7_INTERCEPTOR_FLIGHT_SOURCES)))
# Z8 to Z10 share one source family; the Z10 candidate's inputs aren't always
# prepared, so it stays out of the aggregate target.
$(eval $(call MT_PROBE,z10,ZERG_Z10_RELEASE_FLAGS,ZERG_Z10_CANONICAL_INPUTS,19/20/21))
.PHONY: sc64-mission-text-arm
sc64-mission-text-arm: $(filter-out %-z10.gba,$(MT_PROBE_ROMS))

# The box in the shipped game loop, on the Terran product flags.
build/test-sc64-mission-text-game: tests/sc64_mission_text_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile mk/mission-text/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-mission-text-game
test-sc64-mission-text-game: build/test-sc64-mission-text-game
	./build/test-sc64-mission-text-game
test-sc64-terran-full-campaign: test-sc64-mission-text-game
