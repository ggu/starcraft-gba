# SPDX-License-Identifier: GPL-3.0-or-later
# Add with -f Makefile -f tests/sc64-terran-carrier-envelope/Makefile.
.PHONY: test-sc64-terran-carrier-envelope
test-sc64-terran-carrier-envelope: build/test-sc64-terran-carrier-envelope
	./build/test-sc64-terran-carrier-envelope
build/test-sc64-terran-carrier-envelope: tests/sc64_terran_carrier_envelope.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(COMMON_HEADERS) common/gba.c | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(TERRAN_ROM_FLAGS) -DSC64_INTERCEPTOR_COMPACT_CODEC $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
