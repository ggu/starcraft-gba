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

# A structure change keeps the bytes of complete point fields. Each use is
# validated against the cells opened since the field was flooded, so every
# answer is the one a fresh flood gives, and cache residency is unchanged; only
# the re-flood cost goes away. Empty this to compare a canonical ROM that
# floods every field again after a grid change.
CANONICAL_ROUTE_KEEP_FLAGS ?= -DSC64_ROUTE_KEEP_FIELDS

# After a structure change, update only the walkability cells its footprint can
# have changed instead of rebuilding the whole grid (race maps only). The
# result is the grid a full rebuild gives. Empty this to compare a canonical
# ROM with the full rebuild; add -DSC64_PATH_GRID_VERIFY to a host build to
# compare every incremental update with a full rebuild.
CANONICAL_PATH_GRID_UPDATE_FLAGS ?= -DSC64_PATH_GRID_INCREMENTAL

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

# Randomized differential test of the physical field bank. The reference build
# is the fixed layout (no ACTIVE_STRIDE, no KEEP_FIELDS); the candidates must
# give the same answers and leave the same logical cache residency after every
# query while flooding less. Variant A is the canonical configuration (packed
# grid, monotone probe), with and without survival across grid changes;
# variant B uses a byte grid, decay ages and no probe.
ROUTE_KEEP_TEST_BASE = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -I.
ROUTE_KEEP_TEST_A = -DSC64_ROUTE_PACK3 -DSC64_LARGE_MAPS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_ROUTE_VISIT_GRID -DSC64_ROUTE_WORD_CLEAR -DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_NEW_CELL -DSC64_ROUTE_MONOTONE_PROBE -DSC64_IMPORTED_MISSION -DSC64_REACHABLE_FORMATION -DTEST_PACKED_GRID -DSC64_ROUTE_GRID_CELLS_PER_BYTE=8
ROUTE_KEEP_TEST_B = -DSC64_ROUTE_PACK3 -DSC64_LARGE_MAPS -DSC64_ROUTE_FLOOD_SPLIT -DSC64_ROUTE_DECAY_FREQ -DSC64_IMPORTED_MISSION -DSC64_REACHABLE_FORMATION
ROUTE_KEEP_TEST_DEPS = tests/sc64_route_keep_fields.c starcraft/sc64_routes.h starcraft/sc64_depot_goal.h starcraft/sc64_route_monotone_probe.h starcraft/sc64_map_math.h Makefile mk/combat-perf/Makefile
build/test-sc64-route-keep-fields-a-ref: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_A) $< -o $@
build/test-sc64-route-keep-fields-a-stride: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_A) -DSC64_ROUTE_ACTIVE_STRIDE $< -o $@
build/test-sc64-route-keep-fields-a: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_A) -DSC64_ROUTE_ACTIVE_STRIDE -DSC64_ROUTE_KEEP_FIELDS $< -o $@
build/test-sc64-route-keep-fields-b-ref: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_B) $< -o $@
build/test-sc64-route-keep-fields-b: $(ROUTE_KEEP_TEST_DEPS) | build
	$(CLANG) $(ROUTE_KEEP_TEST_BASE) $(ROUTE_KEEP_TEST_B) -DSC64_ROUTE_ACTIVE_STRIDE -DSC64_ROUTE_KEEP_FIELDS $< -o $@
.PHONY: test-sc64-route-keep-fields
test-sc64-route-keep-fields: build/test-sc64-route-keep-fields-a-ref build/test-sc64-route-keep-fields-a-stride build/test-sc64-route-keep-fields-a build/test-sc64-route-keep-fields-b-ref build/test-sc64-route-keep-fields-b
	./build/test-sc64-route-keep-fields-a-ref --emit build/route-keep-a.ref
	./build/test-sc64-route-keep-fields-a-stride --compare build/route-keep-a.ref
	./build/test-sc64-route-keep-fields-a --compare build/route-keep-a.ref
	./build/test-sc64-route-keep-fields-b-ref --emit build/route-keep-b.ref
	./build/test-sc64-route-keep-fields-b --compare build/route-keep-b.ref
test-starcraft: test-sc64-route-keep-fields

# Randomized differential test of the incremental walkability update against
# the whole-grid rebuild it replaces, over the footprint kinds of each product
# family. The -full builds run the same scenarios with the update disabled.
PATH_GRID_TEST_BASE = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter -fsanitize=address,undefined
PATH_GRID_TEST_DEPS = tests/sc64_path_grid_incremental.c starcraft/path_grid_build.h Makefile mk/combat-perf/Makefile
PATH_GRID_VARIANTS = terran zerg scenario plain
define PATH_GRID_VARIANT_RULES
build/test-sc64-path-grid-$(1): $$(PATH_GRID_TEST_DEPS) | build
	$$(CLANG) $$(PATH_GRID_TEST_BASE) -DVARIANT_$(2) $$< -o $$@
build/test-sc64-path-grid-$(1)-full: $$(PATH_GRID_TEST_DEPS) | build
	$$(CLANG) $$(PATH_GRID_TEST_BASE) -DVARIANT_$(2) -DNO_INCREMENTAL $$< -o $$@
endef
$(eval $(call PATH_GRID_VARIANT_RULES,terran,TERRAN))
$(eval $(call PATH_GRID_VARIANT_RULES,zerg,ZERG))
$(eval $(call PATH_GRID_VARIANT_RULES,scenario,SCENARIO))
$(eval $(call PATH_GRID_VARIANT_RULES,plain,PLAIN))
.PHONY: test-sc64-path-grid-incremental
test-sc64-path-grid-incremental: $(foreach v,$(PATH_GRID_VARIANTS),build/test-sc64-path-grid-$(v) build/test-sc64-path-grid-$(v)-full)
	$(foreach v,$(PATH_GRID_VARIANTS),./build/test-sc64-path-grid-$(v) && ./build/test-sc64-path-grid-$(v)-full && ) true
test-starcraft: test-sc64-path-grid-incremental

# The 32-bit fast path of the depot goal-cell test against the original 64-bit
# computation, over in-range and out-of-range arguments.
build/test-sc64-depot-cell-point-diff: tests/sc64_depot_cell_point_diff.c starcraft/sc64_depot_goal.h mk/combat-perf/Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-depot-cell-point-diff
test-sc64-depot-cell-point-diff: build/test-sc64-depot-cell-point-diff
	./build/test-sc64-depot-cell-point-diff
test-starcraft: test-sc64-depot-cell-point-diff
