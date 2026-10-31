# SPDX-License-Identifier: GPL-3.0-or-later
# Optional actual-P8 actor-pool differential; requires prepared local inputs.
.PHONY: test-sc64-p8-live-condition
build/test-sc64-p8-live-condition: tests/sc64_p8_live_condition_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P8_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P8_FLAGS) -DSC64_P8_LIVE_CONDITION_PROBE $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-sc64-p8-live-condition: build/test-sc64-p8-live-condition
	./build/test-sc64-p8-live-condition
