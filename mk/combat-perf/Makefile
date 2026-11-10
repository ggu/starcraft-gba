# Shared performance options of the race ROMs, and their optional checks.
# SPDX-License-Identifier: GPL-3.0-or-later

# Flip mode-4 pages from the VBlank interrupt so the next update overlaps the
# wait for VBlank. Drawing and simulation are unchanged; only the time at
# which a finished page appears moves. It takes effect in ROMs with the
# sampled-audio interrupt; host builds and other ROMs keep the polled gate.
SC64_PRESENT_FLAGS ?= -DGBA_ASYNC_PRESENT

# Size cached route fields to the active map, so the fixed route bank holds
# more of them on maps smaller than 128 by 128 (56 on the 64-by-64 custom
# maps instead of 14). Answers are unchanged; fewer fields are rebuilt.
# Empty this to compare a canonical ROM with the fixed 14-field layout.
CANONICAL_ROUTE_STRIDE_FLAGS ?= -DSC64_ROUTE_ACTIVE_STRIDE

# Optional differential of the per-profile tick cache, the 32-bit flight and
# position paths against retained earlier code, over every prepared profile.
.PHONY: test-sc64-weapon-source-ticks-bank
test-sc64-weapon-source-ticks-bank: | build
	@if test ! -f "$(WEAPON_SOURCE_HEADER)"; then \
		echo "SKIP: prepared weapon source bank not present at $(WEAPON_SOURCE_HEADER)"; \
	else \
		$(CLANG) -std=c11 -O2 -g -fsanitize=address,undefined -Wall -Wextra -Werror -Wno-unused-function -I"$(WEAPON_SOURCE_DIR)" -Istarcraft tests/sc64_weapon_source_ticks_bank.c -o build/test-sc64-weapon-source-ticks-bank && \
		./build/test-sc64-weapon-source-ticks-bank; \
	fi

# Randomized differential test of kept (tainted) route fields. Variant A is the
# canonical configuration (packed grid, active stride, monotone probe); variant
# B uses the fixed bank, a byte grid and no probe. Each variant runs the same
# scenario sequence without the macro to count floods, then with it.
ROUTE_KEEP_TEST_BASE = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -I.
ROUTE_KEEP_TEST_A = -DSC64_ROUTE_PACK3 -DSC64_LARGE_MAPS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_ROUTE_ACTIVE_STRIDE -DSC64_ROUTE_VISIT_GRID -DSC64_ROUTE_WORD_CLEAR -DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_NEW_CELL -DSC64_ROUTE_MONOTONE_PROBE -DSC64_IMPORTED_MISSION -DSC64_REACHABLE_FORMATION -DTEST_PACKED_GRID -DSC64_ROUTE_GRID_CELLS_PER_BYTE=8
ROUTE_KEEP_TEST_B = -DSC64_ROUTE_PACK3 -DSC64_LARGE_MAPS -DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_DECAY_FREQ -DSC64_IMPORTED_MISSION -DSC64_REACHABLE_FORMATION
ROUTE_KEEP_TEST_DEPS = tests/sc64_route_keep_fields.c starcraft/sc64_routes.h starcraft/sc64_depot_goal.h starcraft/sc64_route_monotone_probe.h starcraft/sc64_map_math.h Makefile
build/test-sc64-route-keep-fields-a: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_A) -DSC64_ROUTE_KEEP_FIELDS $< -o $@
build/test-sc64-route-keep-fields-a-base: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_A) $< -o $@
build/test-sc64-route-keep-fields-b: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_B) -DSC64_ROUTE_KEEP_FIELDS $< -o $@
build/test-sc64-route-keep-fields-b-base: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_B) $< -o $@
.PHONY: test-sc64-route-keep-fields
test-sc64-route-keep-fields: build/test-sc64-route-keep-fields-a build/test-sc64-route-keep-fields-a-base build/test-sc64-route-keep-fields-b build/test-sc64-route-keep-fields-b-base
	./build/test-sc64-route-keep-fields-a-base --emit build/route-keep-a.base
	./build/test-sc64-route-keep-fields-a --compare build/route-keep-a.base
	./build/test-sc64-route-keep-fields-b-base --emit build/route-keep-b.base
	./build/test-sc64-route-keep-fields-b --compare build/route-keep-b.base
test-starcraft: test-sc64-route-keep-fields
