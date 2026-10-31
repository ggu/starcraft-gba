# SPDX-License-Identifier: GPL-3.0-or-later
# Asset-free pixel and OBJ ownership checks through the real host renderer.
TERRAIN_STORAGE_FLAGS = -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_TERRAIN_CACHE_REAL_GFX -DSC64_TERRAIN_SCROLL -Icommon
TERRAIN_STORAGE_DEPS = tests/sc64_terrain_cache.c starcraft/sc64_terrain_cache.h starcraft/sc64_obj_vram.h starcraft/sc64_interceptor_pool_codec.h common/gba.c common/gba.h Makefile tests/terrain-storage/Makefile
build/test-terrain-storage-ram: $(TERRAIN_STORAGE_DEPS) | build
	$(CLANG) $(TERRAIN_STORAGE_FLAGS) $< common/gba.c -o $@
build/test-terrain-storage-vram: $(TERRAIN_STORAGE_DEPS) | build
	$(CLANG) $(TERRAIN_STORAGE_FLAGS) -DSC64_TERRAIN_CACHE_VRAM $< common/gba.c -o $@
build/test-terrain-storage-arena: $(TERRAIN_STORAGE_DEPS) | build
	$(CLANG) $(TERRAIN_STORAGE_FLAGS) -DSC64_TERRAIN_CACHE_VRAM -DSC64_P7_PROJECTILES_ENABLED $< common/gba.c -o $@
.PHONY: test-terrain-storage
test-terrain-storage: build/test-terrain-storage-ram build/test-terrain-storage-vram build/test-terrain-storage-arena
	./build/test-terrain-storage-ram
	./build/test-terrain-storage-vram
	./build/test-terrain-storage-arena
