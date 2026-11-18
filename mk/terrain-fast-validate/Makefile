# SPDX-License-Identifier: GPL-3.0-or-later
# Opt-in SC64_TERRAIN_FAST_VALIDATE (cheaper per-frame terrain-cache
# validation with the same results). Asset-free host checks:
#   test-terrain-fast-validate        differential test, reference vs. fast side
#   test-terrain-fast-validate-cache  the existing terrain-cache, scroll,
#                                     key-once, soft-fog and dirty-capture
#                                     oracles rebuilt with the flag on

TFV_CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -DSC64_IMPORTED_MISSION -DSC64_STATIC_KEY_CULL
TFV_INPUTS := tests/sc64_terrain_fast_validate.c tests/sc64_terrain_fast_validate_side.c tests/sc64_terrain_fast_validate.h starcraft/sc64_terrain_cache.h starcraft/sc64_static_key.h starcraft/sc64_static_key_fast.h starcraft/sc64_fog.h common/gba.c common/gba.h mk/terrain-fast-validate/Makefile
TFV_PACKED := -DFV_PACKED_GRID -DSC64_FOG_TEST_PACKED

# $(1) variant name, $(2) profile flags shared by both sides.
define TFV_VARIANT
build/test-terrain-fast-validate-$(1): $$(TFV_INPUTS) | build
	$$(CLANG) $$(TFV_CFLAGS) $(2) -c tests/sc64_terrain_fast_validate_side.c -o $$@-ref.o
	$$(CLANG) $$(TFV_CFLAGS) $(2) -DFV_SIDE_FAST -DSC64_TERRAIN_FAST_VALIDATE -c tests/sc64_terrain_fast_validate_side.c -o $$@-fast.o
	$$(CLANG) $$(TFV_CFLAGS) $(2) tests/sc64_terrain_fast_validate.c $$@-ref.o $$@-fast.o common/gba.c -o $$@
TFV_TESTS += build/test-terrain-fast-validate-$(1)
endef
# Zerg Z10-like and Showcase-like profiles, then the other cache shapes.
$(eval $(call TFV_VARIANT,z10,-DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_SCROLL -DSC64_SOFT_FOG -DSC64_ZP_FULL_STORAGE -DSC64_RACES -DSC64_Z5_RUNTIME -DSC64_P6_RUNTIME -DSC64_Z9_RUNTIME $(TFV_PACKED)))
$(eval $(call TFV_VARIANT,showcase,-DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_SCROLL -DSC64_SOFT_FOG -DSC64_ZP_FULL_STORAGE -DSC64_RACES -DSC64_P6_RUNTIME -DSC64_T4_SCENARIO_RUNTIME $(TFV_PACKED)))
$(eval $(call TFV_VARIANT,soft-noscroll,-DSC64_RENDER_KEY_ONCE -DSC64_SOFT_FOG -DSC64_RACES $(TFV_PACKED)))
$(eval $(call TFV_VARIANT,hard-scroll,-DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_SCROLL))
$(eval $(call TFV_VARIANT,hard-plain-percall,))
$(eval $(call TFV_VARIANT,soft-scroll-percall,-DSC64_TERRAIN_SCROLL -DSC64_SOFT_FOG -DSC64_RACES -DSC64_Z9_RUNTIME))
$(eval $(call TFV_VARIANT,soft-rowmask,-DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_SOFT_FOG -DSC64_RACES $(TFV_PACKED)))
$(eval $(call TFV_VARIANT,hard-rowmask,-DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_RACES $(TFV_PACKED)))
$(eval $(call TFV_VARIANT,shade-only,-DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_SCROLL -DSC64_SOFT_FOG -DSC64_SOFT_FOG_SHADE_ONLY -DSC64_RACES $(TFV_PACKED)))
$(eval $(call TFV_VARIANT,diag-scroll,-DSC64_RENDER_KEY_ONCE -DSC64_TERRAIN_SCROLL -DSC64_SOFT_FOG -DSC64_DIAGNOSTIC_TERRAIN -DSC64_ZP_FULL_STORAGE -DSC64_RACES $(TFV_PACKED)))
$(eval $(call TFV_VARIANT,diag-plain,-DSC64_RENDER_KEY_ONCE -DSC64_SOFT_FOG -DSC64_DIAGNOSTIC_TERRAIN))

.PHONY: test-terrain-fast-validate
test-terrain-fast-validate: $(TFV_TESTS)
	@set -e; for t in $(TFV_TESTS); do echo "$$t"; ./$$t; done

# The existing cache oracles (tests/sc64_terrain_cache.c), flag on.
TFV_CACHE_FLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DSC64_TERRAIN_FAST_VALIDATE
TFV_REAL := -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -Icommon
TFV_CACHE_INPUTS := tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_static_dirty_rows.h starcraft/sc64_fog.h starcraft/sc64_obj_vram.h common/gba.h common/gba.c mk/terrain-fast-validate/Makefile
# $(1) variant name, $(2) flags, $(3) extra sources.
define TFV_CACHE_VARIANT
build/test-terrain-cache-fv-$(1): $$(TFV_CACHE_INPUTS) | build
	$$(CLANG) $$(TFV_CACHE_FLAGS) $(2) tests/sc64_terrain_cache.c $(3) -o $$@
TFV_CACHE_TESTS += build/test-terrain-cache-fv-$(1)
endef
$(eval $(call TFV_CACHE_VARIANT,cache,,))
$(eval $(call TFV_CACHE_VARIANT,scroll,$(TFV_REAL) -DSC64_TERRAIN_SCROLL,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,key-once,-DSC64_IMPORTED_MISSION -DSC64_RENDER_KEY_ONCE,))
$(eval $(call TFV_CACHE_VARIANT,key-once-scroll,$(TFV_REAL) -DSC64_TERRAIN_SCROLL -DSC64_IMPORTED_MISSION -DSC64_RENDER_KEY_ONCE,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,dirty-capture,$(TFV_REAL) -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_TERRAIN_CACHE_VRAM -DSC64_TERRAIN_DIRTY_CAPTURE -DSC64_P7_PROJECTILES_ENABLED -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -DSC64_FOG_TEST_REAL_RASTER,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,soft-fog,$(TFV_REAL) -DSC64_SOFT_FOG,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,soft-fog-scroll,$(TFV_REAL) -DSC64_TERRAIN_SCROLL -DSC64_SOFT_FOG,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,soft-fog-rowmask,$(TFV_REAL) -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,soft-fog-real-raster-rowmask,$(TFV_REAL) -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -DSC64_FOG_TEST_REAL_RASTER,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,soft-fog-vram,$(TFV_REAL) -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_TERRAIN_CACHE_VRAM -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -DSC64_FOG_TEST_REAL_RASTER,common/gba.c))
$(eval $(call TFV_CACHE_VARIANT,soft-fog-vram-p7,$(TFV_REAL) -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_TERRAIN_CACHE_VRAM -DSC64_P7_PROJECTILES_ENABLED -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -DSC64_FOG_TEST_REAL_RASTER,common/gba.c))

.PHONY: test-terrain-fast-validate-cache
test-terrain-fast-validate-cache: $(TFV_CACHE_TESTS)
	@set -e; for t in $(TFV_CACHE_TESTS); do echo "$$t"; ./$$t; done
test: test-terrain-fast-validate test-terrain-fast-validate-cache
