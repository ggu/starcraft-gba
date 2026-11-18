# Large unit tables: the delta-coded unit save stream and a Zerg profile whose
# unit table holds 408 slots (400 Zerglings for one player at 200 supply).
# SPDX-License-Identifier: GPL-3.0-or-later
#
# SC64_ZP_UNIT_SLOTS sizes the unit table of a full-storage profile (240 by
# default). Release ROMs keep 240; their code and RAM are unchanged. The Swarm
# profile is the Zerg release with 408 slots: 16-bit unit numbers, nine-bit
# bucket heads, ten-bit visibility counts and actor references, and stream
# saves under their own tags.
UNIT_TABLE_SWARM_SLOTS ?= 408
UNIT_TABLE_SWARM_FLAGS = $(ZERG_Z10_RELEASE_FLAGS) $(Z10_BUILDING_WORDS_FLAGS) -DSC64_ZP_UNIT_SLOTS=$(UNIT_TABLE_SWARM_SLOTS)
UNIT_TABLE_DEPS = $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.ld Makefile mk/unit-table/Makefile $(if $(Z10_BUILDING_WORDS_FLAGS),$(SC64_BUILDING_WORDS_HEADER))

# Asset-free codec checks (part of ./dev test).
build/test-sc64-zp-unit-stream: tests/sc64_zp_unit_stream.c starcraft/sc64_zp_unit_stream.h mk/unit-table/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-zp-unit-stream
test-sc64-zp-unit-stream: build/test-sc64-zp-unit-stream
	./build/test-sc64-zp-unit-stream
test: test-sc64-zp-unit-stream

# Swarm ROM: the Zerg release engine with the large table.
build/starcraft-zerg-swarm.elf: $(UNIT_TABLE_DEPS) | build
	$(CC) $(CFLAGS) $(UNIT_TABLE_SWARM_FLAGS) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-swarm.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Swarm-Transport1-GBA.gba: build/starcraft-zerg-swarm.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC ZERG SWRM' SCZS
.PHONY: zerg-swarm
zerg-swarm: build/StarCraft-Zerg-Swarm-Transport1-GBA.gba

# Measurement-only crowd scenes (sc64_crowd_probe.h): N Zerglings into a
# Marine block, on the Swarm table and on the release table.
build/crowd-probe-%.elf: $(UNIT_TABLE_DEPS) | build
	$(CC) $(CFLAGS) $(if $(findstring -t240,$*),$(ZERG_Z10_RELEASE_FLAGS) $(Z10_BUILDING_WORDS_FLAGS),$(UNIT_TABLE_SWARM_FLAGS)) -DSC64_CROWD_PROBE=$(firstword $(subst -, ,$*)) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/crowd-probe-$*.map -lgcc -o $@
.PRECIOUS: build/crowd-probe-%.elf
build/StarCraft-Crowd-%.gba: build/crowd-probe-%.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC CROWD' SCCR
.PHONY: crowd-probes
crowd-probes: build/StarCraft-Crowd-200-t240.gba build/StarCraft-Crowd-200.gba build/StarCraft-Crowd-400.gba

# HOST: 400 Zerglings, bucket invariants, pause-menu save and identical resume.
build/test-sc64-zerg-swarm: tests/sc64_zerg_swarm_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) mk/unit-table/Makefile Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(filter-out -DGBA_NO_DEBUG_INPUTS,$(UNIT_TABLE_SWARM_FLAGS)) -DSC64_CROWD_PROBE=400 -DSC64_CROWD_PROBE_MANUAL $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-zerg-swarm
test-sc64-zerg-swarm: build/test-sc64-zerg-swarm
	./build/test-sc64-zerg-swarm
