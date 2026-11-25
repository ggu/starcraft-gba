# SPDX-License-Identifier: GPL-3.0-or-later
# Protoss shield hit overlay on the real engine. Needs the top-level Makefile's
# flag and input variables, so it is included last. Only products compiled
# with the source weapon bank have the overlay.
ifneq ($(filter -DSC64_WEAPON_SOURCE_VFX,$(TERRAN_ROM_FLAGS)),)
build/test-sc64-shield-flash-game: tests/sc64_shield_flash_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(SOURCE_WEAPON_INPUTS) $(TANK_SOURCE_HEADER) $(CANONICAL_DEATH_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/shield-flash/Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-shield-flash-game
test-sc64-shield-flash-game: build/test-sc64-shield-flash-game
	./build/test-sc64-shield-flash-game
test-sc64-terran-full-campaign: test-sc64-shield-flash-game
endif
