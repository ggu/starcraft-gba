# Large unit tables: measurement scenes and, from later steps, the unit
# save stream and a large-table Zerg profile.
# SPDX-License-Identifier: GPL-3.0-or-later
UNIT_TABLE_DEPS = $(COMMON) $(COMMON_HEADERS) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.ld Makefile mk/unit-table/Makefile $(if $(Z10_BUILDING_WORDS_FLAGS),$(SC64_BUILDING_WORDS_HEADER))

# Measurement-only crowd scenes (sc64_crowd_probe.h): N Zerglings into a
# Marine block on the Zerg release engine (build/StarCraft-Crowd-200-t240.gba).
build/crowd-probe-%.elf: $(UNIT_TABLE_DEPS) | build
	$(CC) $(CFLAGS) $(ZERG_Z10_RELEASE_FLAGS) $(Z10_BUILDING_WORDS_FLAGS) -DSC64_CROWD_PROBE=$(firstword $(subst -, ,$*)) $(COMMON) $(STARCRAFT) $(LDFLAGS) -Wl,-Map,build/crowd-probe-$*.map -lgcc -o $@
.PRECIOUS: build/crowd-probe-%.elf
build/StarCraft-Crowd-%.gba: build/crowd-probe-%.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC CROWD' SCCR
