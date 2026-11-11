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
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DGBA_SCENE_CLIP -Icommon $< common/gba.c -o $@
build/test-sc64-fog-no-clip: tests/sc64_fog.c starcraft/sc64_fog.h common/gba.c common/gba.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $< common/gba.c -o $@
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
