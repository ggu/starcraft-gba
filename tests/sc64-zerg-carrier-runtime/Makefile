# SPDX-License-Identifier: GPL-3.0-or-later
# Add with -f Makefile -f tests/sc64-zerg-carrier-runtime/Makefile.
ZERG_CARRIER_PROBE_FLAGS := $(SC64_CARRIER_RELEASE_FLAGS)
ZERG_CARRIER_PROBE_DEPS = $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) $(RACES_INPUTS) $(COMMON_HEADERS) Makefile mk/carrier-release/flags/Makefile tests/sc64-zerg-carrier-runtime/Makefile
.PHONY: test-sc64-zerg-carrier-runtime
test-sc64-zerg-carrier-runtime: build/test-sc64-zerg-carrier-runtime
	./build/test-sc64-zerg-carrier-runtime
build/test-sc64-zerg-carrier-runtime: tests/sc64_zerg_carrier_runtime.c $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(ZERG_CARRIER_PROBE_DEPS) common/gba.c | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(ZERG_Z10_RELEASE_FLAGS) $(ZERG_CARRIER_PROBE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/starcraft-zerg-carrier-probe.elf: $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(ZERG_CARRIER_PROBE_DEPS) common/gba.ld | build
	$(CC) $(CFLAGS) $(ZERG_Z10_RELEASE_FLAGS) $(ZERG_CARRIER_PROBE_FLAGS) $(COMMON) $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(LDFLAGS) -Wl,-Map,build/starcraft-zerg-carrier-probe.map -lgcc -o $@
	$(SIZE) $@
build/StarCraft-Zerg-Z10-Carrier-Probe.gba: build/starcraft-zerg-carrier-probe.elf tools/fix_rom.py
	$(OBJCOPY) -O binary $< $@
	$(PYTHON) tools/fix_rom.py $@ 'SC Z C PROBE' SCZA
