# Isolated typed-arena experiment; canonical flags remain unchanged.
.PHONY: test-sc64-shared-projectile-stage
build/test-sc64-shared-projectile-stage: tests/sc64_shared_projectile_stage_game.c starcraft/sc64_shared_projectile_stage.h $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_INTERCEPTOR_FLIGHT_FLAGS) -DSC64_SHARED_PROJECTILE_STAGE $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
test-sc64-shared-projectile-stage: build/test-sc64-shared-projectile-stage
	./build/test-sc64-shared-projectile-stage

# Combined live-family contention fixture; the art renderer is unchanged and
# intentionally outside this focused host test.
.PHONY: test-sc64-shared-yamato-interceptor
build/test-sc64-shared-yamato-interceptor: tests/sc64_shared_yamato_interceptor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(filter-out -DSC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT,$(BC_FLAGS)) $(P7_INTERCEPTOR_FLIGHT_FLAGS) -DSC64_SHARED_PROJECTILE_STAGE $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-sc64-shared-yamato-interceptor: build/test-sc64-shared-yamato-interceptor
	./build/test-sc64-shared-yamato-interceptor

# Explicit experimental combined-family fixture; production flags stay off.
.PHONY: test-sc64-shared-scarab-interceptor
build/test-sc64-shared-scarab-interceptor: tests/sc64_shared_scarab_interceptor_game.c tests/sc64_shared_yamato_interceptor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P7_ARBITER_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(filter-out -DSC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT,$(BC_FLAGS)) $(P7_INTERCEPTOR_FLIGHT_FLAGS) -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE -DSC64_WEAPON_SOURCE_VFX -I$(WEAPON_SOURCE_DIR) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-sc64-shared-scarab-interceptor: build/test-sc64-shared-scarab-interceptor
	./build/test-sc64-shared-scarab-interceptor

.PHONY: test-sc64-shared-scarab-arena
build/test-sc64-shared-scarab-arena: tests/sc64_scarab_shared_arena_game.c tests/sc64_impact_events_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CANONICAL_LOOKUP_INPUTS) | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_NAMESPACE_STAGE -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE $(TERRAN_ROM_FLAGS) $< tests/sc64_yamato_host_sram_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@
test-sc64-shared-scarab-arena: build/test-sc64-shared-scarab-arena
	./build/test-sc64-shared-scarab-arena

# Source-profile ordinary allocation experiment; all stage gates remain off in
# products. The live-bullet arena is separate from the legacy 32 impact rows.
SC64_SHARED_ORDINARY_FLAGS := -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_WEAPON_SOURCE_VFX -I$(WEAPON_SOURCE_DIR)
.PHONY: test-sc64-shared-ordinary-projectiles
SC64_SHARED_ORDINARY_INPUTS := $(P7_ARBITER_INPUTS) $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c
build/test-sc64-shared-ordinary-projectiles: tests/sc64_shared_ordinary_projectile_game.c tests/sc64_shared_yamato_interceptor_game.c $(SC64_SHARED_ORDINARY_INPUTS) | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(filter-out -DSC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT,$(BC_FLAGS)) $(P7_INTERCEPTOR_FLIGHT_FLAGS) $(SC64_SHARED_ORDINARY_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-sc64-shared-ordinary-projectiles: build/test-sc64-shared-ordinary-projectiles
	./build/test-sc64-shared-ordinary-projectiles
.PHONY: test-sc64-shared-ordinary-scarab-order
build/test-sc64-shared-ordinary-scarab-order: tests/sc64_shared_ordinary_scarab_order_game.c tests/sc64_shared_yamato_interceptor_game.c $(SC64_SHARED_ORDINARY_INPUTS) | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(filter-out -DSC64_BATTLECRUISER_YAMATO_ART_EXPERIMENT,$(BC_FLAGS)) $(P7_INTERCEPTOR_FLIGHT_FLAGS) $(SC64_SHARED_ORDINARY_FLAGS) -DSC64_SCARAB_RUNTIME -DSC64_SCARAB_FLIGHT_CONTROL_PROBE -DSC64_SCARAB_PACKED_MOTION_PROBE -DSC64_SCARAB_AUTONOMOUS_PROBE -DSC64_SCARAB_WEAPON_PROBE $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-sc64-shared-ordinary-scarab-order: build/test-sc64-shared-ordinary-scarab-order
	./build/test-sc64-shared-ordinary-scarab-order
.PHONY: test-sc64-shared-ordinary-z6
build/test-sc64-shared-ordinary-z6: tests/sc64_shared_ordinary_z6_game.c tests/sc64_z6_hallucination.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wno-unused-function -Wno-unused-variable $(ZERG_ROM_FLAGS) -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
test-sc64-shared-ordinary-z6: build/test-sc64-shared-ordinary-z6
	./build/test-sc64-shared-ordinary-z6
