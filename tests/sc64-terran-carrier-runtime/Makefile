# SPDX-License-Identifier: GPL-3.0-or-later
# Add with -f Makefile -f tests/sc64-terran-carrier-runtime/Makefile.
TERRAN_CARRIER_PROBE_FLAGS := -DSC64_TERRAN_CARRIER_PROBE -DSC64_INTERCEPTOR_COMPACT_CODEC -DSC64_P7_PROJECTILES_ENABLED -DSC64_P7_CARRIER_TRAINING_ENABLED -DSC64_P7_INTERCEPTOR_FLIGHT_ENABLED
TERRAN_CARRIER_PROBE_DEPS = $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(RACES_INPUTS) $(COMMON_HEADERS) Makefile tests/sc64-terran-carrier-runtime/Makefile
.PHONY: test-sc64-terran-carrier-runtime
test-sc64-terran-carrier-runtime: build/test-sc64-terran-carrier-runtime
	./build/test-sc64-terran-carrier-runtime
build/test-sc64-terran-carrier-runtime: tests/sc64_terran_carrier_runtime.c $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(TERRAN_CARRIER_PROBE_DEPS) common/gba.c | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(TERRAN_ROM_FLAGS) $(TERRAN_CARRIER_PROBE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/starcraft-terran-carrier-probe.elf: $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(TERRAN_CARRIER_PROBE_DEPS) common/gba.ld | build
	$(CC) $(CFLAGS) $(TERRAN_ROM_FLAGS) $(TERRAN_CARRIER_PROBE_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-terran-carrier-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Terran-Carrier-Probe.gba: build/starcraft-terran-carrier-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC T C PROBE' SCT8
