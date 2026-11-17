# Included by the root Makefile; this file is not a standalone build.
# Optional save-context coverage: the same production vectors and source
# dependencies as the default matrix, with independent binaries and one flag.
.PHONY: test-sc64-shared-mechanics-ui-context test-sc64-campaign-ui-context
build/test-sc64-shared-mechanics-ui-context-terran: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(TERRAN_FULL_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(TERRAN_ROM_FLAGS) -DSC64_SAVE_UI_CONTEXT $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-shared-mechanics-ui-context-zerg: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) $(UNIT_FACING_INPUTS) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(ZERG_ROM_FLAGS) -DSC64_SAVE_UI_CONTEXT $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-shared-mechanics-ui-context-protoss: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P7_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) -DSC64_SAVE_UI_CONTEXT $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-shared-mechanics-ui-context-showcase: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(COMMAND_ICON_HEADER) $(TANK_SOURCE_HEADER) $(SOURCE_WEAPON_INPUTS) $(CANONICAL_DEATH_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(SHOWCASE_FLAGS) -DSC64_SAVE_UI_CONTEXT $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES)) -o $@
build/test-sc64-shared-mechanics-ui-context-races: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(RACES_FLAGS) -DSC64_SAVE_UI_CONTEXT $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
build/test-sc64-shared-mechanics-ui-context-unified: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACES_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(UNIFIED_FLAGS) -DSC64_SAVE_UI_CONTEXT $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
test-sc64-shared-mechanics-ui-context: $(addprefix build/test-sc64-shared-mechanics-ui-context-,terran zerg protoss showcase races unified)
	$(foreach profile,terran zerg protoss showcase races unified,./build/test-sc64-shared-mechanics-ui-context-$(profile) && ) true

# Exact late-campaign vectors are separate opt-in profiles. These retain each
# campaign's complete generated inputs and runtime feature set while testing
# the same UI record codec used by the six-profile mechanics matrix.
build/test-sc64-campaign-ui-context-p10: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(PROTOSS_P10_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile mk/protoss-p9/Makefile mk/protoss-p10/Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(PROTOSS_P10_FLAGS) -DSC64_SAVE_UI_CONTEXT -DSC64_SAVE_UI_FIXTURE_ONLY $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-campaign-ui-context-z10: tests/sc64_shared_mechanics_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z10_CANONICAL_INPUTS) common/gba.c $(COMMON_HEADERS) Makefile tests/sc64-zerg-late/Makefile tests/sc64-save-ui-context/Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) $(ZERG_Z10_CANONICAL_FLAGS) -DSC64_SAVE_UI_CONTEXT -DSC64_SAVE_UI_FIXTURE_ONLY $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
test-sc64-campaign-ui-context: build/test-sc64-campaign-ui-context-p10 build/test-sc64-campaign-ui-context-z10
	./build/test-sc64-campaign-ui-context-p10
	./build/test-sc64-campaign-ui-context-z10
