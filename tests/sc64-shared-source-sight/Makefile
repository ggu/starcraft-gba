# SPDX-License-Identifier: GPL-3.0-or-later
# Optional prepared-profile checks; callers may provide a verified cover overlay.
SC64_SHARED_SIGHT_EXTRA_FLAGS ?=
SC64_SHARED_SIGHT_HOST_FLAGS = $(P8_HOST_FLAGS) $(SC64_SHARED_SIGHT_EXTRA_FLAGS)
SC64_SHARED_SIGHT_HOST_SOURCES = common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c
build/test-sc64-shared-sight-p10: tests/sc64_shared_source_sight_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) $(SOURCE_WEAPON_INPUTS) $(SC64_SHARED_SIGHT_HOST_SOURCES) $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(SC64_SHARED_SIGHT_HOST_FLAGS) $(PROTOSS_P10_BASE_FLAGS) $< $(SC64_SHARED_SIGHT_HOST_SOURCES) -o $@
build/test-sc64-shared-sight-z9: tests/sc64_shared_source_sight_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) $(SOURCE_WEAPON_INPUTS) $(SC64_SHARED_SIGHT_HOST_SOURCES) $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(SC64_SHARED_SIGHT_HOST_FLAGS) $(ZERG_Z9_RELEASE_FLAGS) $< $(SC64_SHARED_SIGHT_HOST_SOURCES) -o $@
build/test-sc64-shared-sight-detector: tests/sc64_p7_owner_mask_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) $(SOURCE_WEAPON_INPUTS) $(SC64_SHARED_SIGHT_HOST_SOURCES) $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(SC64_SHARED_SIGHT_HOST_FLAGS) $(PROTOSS_P10_BASE_FLAGS) $< $(SC64_SHARED_SIGHT_HOST_SOURCES) -o $@
.PHONY: test-sc64-shared-source-sight
test-sc64-shared-source-sight: build/test-sc64-shared-sight-p10 build/test-sc64-shared-sight-z9 build/test-sc64-shared-sight-detector build/test-sc64-shared-sight-noncache
	./build/test-sc64-shared-sight-p10
	./build/test-sc64-shared-sight-z9
	./build/test-sc64-shared-sight-detector
	./build/test-sc64-shared-sight-noncache
# The scalar observer route must normalize the same current-human controller.
SC64_SHARED_SIGHT_NONCACHE_FLAGS = $(P7_RUNTIME_FLAGS)
build/test-sc64-shared-sight-noncache: tests/sc64_shared_sight_noncache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(SOURCE_WEAPON_INPUTS) $(SC64_SHARED_SIGHT_HOST_SOURCES) $(COMMON_HEADERS) Makefile | build
	$(CLANG) $(SC64_SHARED_SIGHT_HOST_FLAGS) $(SC64_SHARED_SIGHT_NONCACHE_FLAGS) $< $(SC64_SHARED_SIGHT_HOST_SOURCES) -o $@
