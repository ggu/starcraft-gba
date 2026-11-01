# Prepared-profile shared colony checks. No private source payloads are tracked.
COLONY_TEST_CFLAGS = -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-z7-colony-update: tests/sc64_z7_colony_update.c tests/sc64_z7_colonies_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z7_CANONICAL_INPUTS) common/gba.c common/gba.h | build
	$(CLANG) $(COLONY_TEST_CFLAGS) $(ZERG_Z7_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z8-colony: tests/sc64_z8_colony_game.c tests/sc64_z8_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z8_CANONICAL_INPUTS) common/gba.c common/gba.h | build
	$(CLANG) $(COLONY_TEST_CFLAGS) $(ZERG_Z8_CANONICAL_FLAGS) $< common/gba.c -o $@
build/test-sc64-z8-colony-derived: tests/sc64_z8_colony_game.c tests/sc64_z8_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z8_CANONICAL_INPUTS) common/gba.c common/gba.h | build
	$(CLANG) $(COLONY_TEST_CFLAGS) $(ZERG_Z8_CANONICAL_FLAGS) -DSC64_Z7_DERIVED_SAVE $< common/gba.c -o $@
build/test-sc64-skirmish-colony: tests/sc64_skirmish_colony.c tests/sc64_skirmish_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) common/gba.c common/gba.h | build
	$(CLANG) $(COLONY_TEST_CFLAGS) $(ZERG_ROM_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
.PHONY: test-sc64-shared-colony
test-sc64-shared-colony: build/test-sc64-z7-colony-update build/test-sc64-z8-colony build/test-sc64-z8-colony-derived build/test-sc64-skirmish-colony
	./build/test-sc64-z7-colony-update
	./build/test-sc64-z8-colony
	./build/test-sc64-z8-colony-derived
	./build/test-sc64-skirmish-colony
