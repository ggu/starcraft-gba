# SPDX-License-Identifier: GPL-3.0-or-later
build/test-sc64-foreground-pick: tests/sc64_z3_frame_coverage.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) common/gba.h common/gba.c Makefile mk/foreground-pick/Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -DSC64_FOREGROUND_PICK -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@

.PHONY: test-sc64-foreground-pick
test-sc64-foreground-pick: build/test-sc64-foreground-pick
	./build/test-sc64-foreground-pick
