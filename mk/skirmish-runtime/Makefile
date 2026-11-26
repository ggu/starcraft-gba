# SPDX-License-Identifier: GPL-3.0-or-later

# Prepared-input integration gate. These tests include the actual engine;
# unlike the catalog checks, they require the corresponding local art banks.
.PHONY: test-sc64-skirmish-runtime
SKIRMISH_RUNTIME_TEST_FLAGS = -std=c11 -O1 -g -fsanitize=address,undefined -DGBA_HOST -Icommon
build/test-sc64-skirmish-runtime-legacy-larva: SKIRMISH_LEGACY_OMIT = -DSC64_CUSTOM_LARVA_EGGS -DSC64_ENGINE_CORE_REQUIRED
build/test-sc64-skirmish-runtime-terran build/test-sc64-skirmish-runtime-legacy-larva: tests/sc64_skirmish_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) common/gba.c Makefile mk/skirmish-runtime/Makefile | build
	$(CLANG) $(SKIRMISH_RUNTIME_TEST_FLAGS) $(filter-out $(SKIRMISH_LEGACY_OMIT),$(TERRAN_ROM_FLAGS)) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-skirmish-runtime-zerg: tests/sc64_skirmish_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CANONICAL_ZERG_PROFILE_INPUTS) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) common/gba.c Makefile mk/skirmish-runtime/Makefile | build
	$(CLANG) $(SKIRMISH_RUNTIME_TEST_FLAGS) $(CANONICAL_ZERG_PROFILE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-skirmish-runtime-protoss: tests/sc64_skirmish_runtime.c $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P7_CANONICAL_INPUTS) common/gba.c Makefile mk/skirmish-runtime/Makefile | build
	$(CLANG) $(SKIRMISH_RUNTIME_TEST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-skirmish-runtime-showcase: tests/sc64_skirmish_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(COMMAND_ICON_HEADER) $(TANK_SOURCE_HEADER) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS) common/gba.c Makefile mk/skirmish-runtime/Makefile | build
	$(CLANG) $(SKIRMISH_RUNTIME_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
test-sc64-skirmish-runtime: build/test-sc64-skirmish-runtime-terran build/test-sc64-skirmish-runtime-zerg build/test-sc64-skirmish-runtime-protoss build/test-sc64-skirmish-runtime-showcase build/test-sc64-skirmish-runtime-legacy-larva
	./build/test-sc64-skirmish-runtime-terran
	./build/test-sc64-skirmish-runtime-zerg
	./build/test-sc64-skirmish-runtime-protoss
	./build/test-sc64-skirmish-runtime-showcase
	./build/test-sc64-skirmish-runtime-legacy-larva
