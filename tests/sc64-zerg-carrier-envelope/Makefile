# SPDX-License-Identifier: GPL-3.0-or-later
# Add with -f Makefile -f tests/sc64-zerg-carrier-envelope/Makefile.
.PHONY: test-sc64-zerg-carrier-envelope
test-sc64-zerg-carrier-envelope: build/test-sc64-zerg-carrier-envelope
	./build/test-sc64-zerg-carrier-envelope
build/test-sc64-zerg-carrier-envelope: tests/sc64_zerg_carrier_envelope.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) $(COMMON_HEADERS) common/gba.c | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(ZERG_Z10_RELEASE_FLAGS) -DSC64_INTERCEPTOR_COMPACT_CODEC $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
