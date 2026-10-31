build/test-sc64-zerg-canonical-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CANONICAL_ZERG_PROFILE_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(CANONICAL_ZERG_PROFILE_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@

# Z7 uses the uncached lifetime path; test its production behavior directly.
build/test-sc64-tracer-profile-zerg: tests/sc64_z7_tracer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CANONICAL_ZERG_PROFILE_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(CANONICAL_ZERG_PROFILE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
