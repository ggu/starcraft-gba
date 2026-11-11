# Soft fog: palette-shaded explored terrain and world-anchored edge masks.
# SPDX-License-Identifier: GPL-3.0-or-later
# CANONICAL_FOG_FLAGS is defined in the top-level Makefile.

.PHONY: test-terrain-soft-fog
build/test-terrain-soft-fog: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_fog.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_SOFT_FOG -Icommon $< common/gba.c -o $@
build/test-terrain-soft-fog-scroll: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_fog.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -DSC64_SOFT_FOG -Icommon $< common/gba.c -o $@
build/test-terrain-soft-fog-rowmask: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_fog.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -Icommon $< common/gba.c -o $@
build/test-terrain-soft-fog-real-raster-rowmask: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_fog.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -DSC64_FOG_TEST_REAL_RASTER -Icommon $< common/gba.c -o $@
build/test-terrain-soft-fog-vram: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_fog.h starcraft/sc64_obj_vram.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_TERRAIN_CACHE_VRAM -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -DSC64_FOG_TEST_REAL_RASTER -Icommon $< common/gba.c -o $@
build/test-terrain-soft-fog-vram-p7: tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_fog.h starcraft/sc64_obj_vram.h common/gba.h common/gba.c | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -DSC64_TERRAIN_ROW_MASKS -DSC64_TERRAIN_CACHE_VRAM -DSC64_P7_PROJECTILES_ENABLED -DSC64_ZP_CAMPAIGN_STORAGE -DSC64_FOG_TEST_PACKED -DSC64_SOFT_FOG -DSC64_FOG_TEST_REAL_RASTER -Icommon $< common/gba.c -o $@
test-terrain-soft-fog: build/test-terrain-soft-fog build/test-terrain-soft-fog-scroll build/test-terrain-soft-fog-rowmask build/test-terrain-soft-fog-real-raster-rowmask build/test-terrain-soft-fog-vram build/test-terrain-soft-fog-vram-p7
	./build/test-terrain-soft-fog
	./build/test-terrain-soft-fog-scroll
	./build/test-terrain-soft-fog-rowmask
	./build/test-terrain-soft-fog-real-raster-rowmask
	./build/test-terrain-soft-fog-vram
	./build/test-terrain-soft-fog-vram-p7
test: test-terrain-soft-fog

# Asset-free ternary fog signature and clipped indexed-cell raster contract.
build/test-sc64-fog: tests/sc64_fog.c starcraft/sc64_fog.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_SOFT_FOG -DGBA_SCENE_CLIP -Icommon $< common/gba.c -o $@
build/test-sc64-fog-no-clip: tests/sc64_fog.c starcraft/sc64_fog.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_SOFT_FOG -Icommon $< common/gba.c -o $@
.PHONY: test-sc64-fog
test-sc64-fog: build/test-sc64-fog build/test-sc64-fog-no-clip
	./build/test-sc64-fog
	./build/test-sc64-fog-no-clip
test: test-sc64-fog

# Pure synthetic RGB555 shade-map contract, independent of GBA runtime/assets.
build/test-gba-fog-palette: tests/gba_fog_palette.c common/gba_fog_palette.h common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Icommon $< -o $@
build/test-gba-fog-palette-runtime: tests/gba_fog_palette.c common/gba_fog_palette.h common/gba.c common/gba.h starcraft/sc64_obj_vram.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_SOFT_FOG -Icommon $< common/gba.c -o $@
build/test-gba-fog-palette-runtime-p7: tests/gba_fog_palette.c common/gba_fog_palette.h common/gba.c common/gba.h starcraft/sc64_obj_vram.h starcraft/sc64_interceptor_pool_codec.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_SOFT_FOG -DSC64_P7_PROJECTILES_ENABLED -Icommon $< common/gba.c -o $@
.PHONY: test-gba-fog-palette
test-gba-fog-palette: build/test-gba-fog-palette build/test-gba-fog-palette-runtime build/test-gba-fog-palette-runtime-p7
	./build/test-gba-fog-palette
	./build/test-gba-fog-palette-runtime
	./build/test-gba-fog-palette-runtime-p7
test: test-gba-fog-palette

# Real rendered frames with the feature on and off (see
# tests/test_sc64_fog_frames.py). Each pair uses one product's own flags; only
# -DSC64_SOFT_FOG differs. Variable names are passed because some canonical
# flags are defined later.
FOG_FRAMES_FLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
define FOG_FRAMES_PROFILE
build/sc64-fog-frames-$(1)-on: tests/sc64_fog_frames.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$($(3)) $$($(4)) $$(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile mk/fog/Makefile | build
	$$(CLANG) $$(FOG_FRAMES_FLAGS) $$($(2)) -DSC64_SOFT_FOG $$< starcraft/sc64_bc_body_art.c $$($(4)) -o $$@
build/sc64-fog-frames-$(1)-off: tests/sc64_fog_frames.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$($(3)) $$($(4)) $$(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile mk/fog/Makefile | build
	$$(CLANG) $$(FOG_FRAMES_FLAGS) $$($(2)) -USC64_SOFT_FOG $$< starcraft/sc64_bc_body_art.c $$($(4)) -o $$@
endef
$(eval $(call FOG_FRAMES_PROFILE,terran,TERRAN_ROM_FLAGS,TERRAN_FULL_INPUTS,))
$(eval $(call FOG_FRAMES_PROFILE,zerg,CANONICAL_ZERG_PROFILE_FLAGS,CANONICAL_ZERG_PROFILE_INPUTS,))
$(eval $(call FOG_FRAMES_PROFILE,protoss,PROTOSS_ROM_FLAGS,PROTOSS_CANONICAL_INPUTS,SKIRMISH_AI_PROTOSS_SOURCES))
