# Shared production fixtures follow the current default Zerg product.
CANONICAL_ZERG_PROFILE_FLAGS = $(ZERG_Z7_CANONICAL_FLAGS)
CANONICAL_ZERG_PROFILE_INPUTS = $(ZERG_Z7_CANONICAL_INPUTS)
build/test-sc64-zerg-canonical-hud: tests/sc64_hud_cache.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CANONICAL_ZERG_PROFILE_INPUTS) common/gba.h common/gba.c Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(CANONICAL_ZERG_PROFILE_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
