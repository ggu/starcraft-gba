# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the Terran, Zerg and Protoss
# campaign rules; it is not a standalone makefile. Run these targets from the
# repository root.

# Win -> result screen -> A -> next mission, in ONE process, for one mission per
# race: an existing ordinary-input controller wins the mission, the fixture
# dismisses any text box with A, checks the result screen and the completed
# bit, presses A, and requires the next catalog mission to start with the right
# id and a world identical to that mission's ordinary menu launch.
# Z2 -> Z3 (Zerg canonical flags), P5 -> P6 (P10 flags), T4 -> T5 (full Terran).
PROGRESSION_HEADER := tests/sc64_campaign_progression.h

build/test-sc64-progression-zerg: tests/sc64_progression_zerg.c tests/race_campaign_run.c $(PROGRESSION_HEADER) $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z9_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/campaign-progression/Makefile tests/sc64-zerg-late/Makefile | build
	$(CLANG) $(Z8_HOST_FLAGS) $(ZERG_Z9_CANONICAL_FLAGS) $< common/gba.c -o $@

build/test-sc64-progression-protoss: tests/sc64_progression_protoss.c tests/sc64_p5_campaign_run.c $(PROGRESSION_HEADER) $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/campaign-progression/Makefile mk/protoss-carried/Makefile | build
	$(PX_CLANG) $< common/gba.c $(PX_SOURCES) -o $@

build/test-sc64-progression-terran: tests/sc64_progression_terran.c tests/sc64_t4_campaign_run.c $(PROGRESSION_HEADER) $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) common/gba.h common/gba.c Makefile mk/campaign-progression/Makefile | build
	$(CLANG) -std=c11 -O2 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon $(TERRAN_ROM_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@

.PHONY: test-sc64-progression-zerg test-sc64-progression-protoss test-sc64-progression-terran test-sc64-campaign-progression
test-sc64-progression-zerg: build/test-sc64-progression-zerg
	./build/test-sc64-progression-zerg
test-sc64-progression-protoss: build/test-sc64-progression-protoss
	./build/test-sc64-progression-protoss
test-sc64-progression-terran: build/test-sc64-progression-terran
	mkdir -p .local/sc64
	./build/test-sc64-progression-terran
test-sc64-campaign-progression: test-sc64-progression-terran test-sc64-progression-zerg test-sc64-progression-protoss

test-sc64-terran-full-campaign: test-sc64-progression-terran
test-sc64-zerg-z9: test-sc64-progression-zerg
test-sc64-protoss-px: test-sc64-progression-protoss
