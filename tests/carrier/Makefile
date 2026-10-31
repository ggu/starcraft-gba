# Gated cross-profile Carrier development fixture; not a production capability.
SHARED_CARRIER_EXPERIMENT_SOURCES = $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES)
define SHARED_CARRIER_EXPERIMENT_FIXTURE
build/test-sc64-shared-carrier-$(1): tests/sc64_shared_carrier_experiment.c $(STARCRAFT) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) $(3) common/gba.c $(COMMON_HEADERS) Makefile tests/carrier/Makefile | build
	$$(CLANG) $$(SHARED_MECHANICS_HOST_FLAGS) $$($(2)) -DSC64_SHARED_CARRIER_EXPERIMENT $$< common/gba.c $$(SHARED_CARRIER_EXPERIMENT_SOURCES) -o $$@
endef
$(eval $(call SHARED_CARRIER_EXPERIMENT_FIXTURE,terran,TERRAN_ROM_FLAGS,$(TERRAN_FULL_INPUTS)))
$(eval $(call SHARED_CARRIER_EXPERIMENT_FIXTURE,zerg,ZERG_ROM_FLAGS,$(ZERG_Z6_INPUTS)))
$(eval $(call SHARED_CARRIER_EXPERIMENT_FIXTURE,protoss,PROTOSS_P7_CANONICAL_FLAGS,$(PROTOSS_P7_CANONICAL_INPUTS)))
$(eval $(call SHARED_CARRIER_EXPERIMENT_FIXTURE,showcase,SHOWCASE_FLAGS,$(SHOWCASE_INPUTS)))

# Scratch-only predecessor-journal probe; both programs are host-only.
define SHARED_CARRIER_LEGACY_FIXTURE
build/test-sc64-carrier-legacy-$(1)-writer: tests/sc64_shared_carrier_legacy_journal.c $$(STARCRAFT) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(wildcard starcraft/*.h) $(3) common/gba.c $$(COMMON_HEADERS) Makefile tests/carrier/Makefile | build
	$$(CLANG) $$(SHARED_MECHANICS_HOST_FLAGS) $$($(2)) $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $$@
build/test-sc64-carrier-legacy-$(1)-reader: tests/sc64_shared_carrier_legacy_journal.c $$(STARCRAFT) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(wildcard starcraft/*.h) $(3) common/gba.c $$(COMMON_HEADERS) Makefile tests/carrier/Makefile | build
	$$(CLANG) $$(SHARED_MECHANICS_HOST_FLAGS) $$($(2)) -DSC64_SHARED_CARRIER_EXPERIMENT $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $$@
endef
$(eval $(call SHARED_CARRIER_LEGACY_FIXTURE,terran,TERRAN_ROM_FLAGS,$(TERRAN_FULL_INPUTS)))
$(eval $(call SHARED_CARRIER_LEGACY_FIXTURE,zerg,ZERG_ROM_FLAGS,$(ZERG_Z6_INPUTS)))
$(eval $(call SHARED_CARRIER_LEGACY_FIXTURE,showcase,SHOWCASE_FLAGS,$(SHOWCASE_INPUTS)))

# Explicitly gated full game save/load entry-point fixture; never a ROM flag.
define SHARED_CARRIER_SAVE_FIXTURE
build/test-sc64-carrier-save-$(1): tests/sc64_shared_carrier_legacy_journal.c $$(STARCRAFT) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) $$(wildcard starcraft/*.h) $(3) common/gba.c $$(COMMON_HEADERS) Makefile tests/carrier/Makefile | build
	$$(CLANG) $$(SHARED_MECHANICS_HOST_FLAGS) $$($(2)) -DSC64_SHARED_CARRIER_EXPERIMENT -DSC64_SHARED_CARRIER_SAVE_EXPERIMENT $$< $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $$@
endef
$(eval $(call SHARED_CARRIER_SAVE_FIXTURE,terran,TERRAN_ROM_FLAGS,$(TERRAN_FULL_INPUTS)))
$(eval $(call SHARED_CARRIER_SAVE_FIXTURE,zerg,ZERG_ROM_FLAGS,$(ZERG_Z6_INPUTS)))
$(eval $(call SHARED_CARRIER_SAVE_FIXTURE,showcase,SHOWCASE_FLAGS,$(SHOWCASE_INPUTS)))

# The source hit bits stay lossless; candidate support follows live capabilities.
build/test-sc64-carrier-save-flags-base: tests/sc64_interceptor_save_flags.c starcraft/sc64_interceptor_pool_codec.h tests/carrier/Makefile | build
	$(CLANG) -std=c11 -O1 -g0 -fsanitize=address,undefined $< -o $@
build/test-sc64-carrier-save-flags-swarm: tests/sc64_interceptor_save_flags.c starcraft/sc64_interceptor_pool_codec.h tests/carrier/Makefile | build
	$(CLANG) -std=c11 -O1 -g0 -fsanitize=address,undefined -DSC64_SHARED_DARK_SWARM_PROBE $< -o $@
.PHONY: test-sc64-carrier-save-flags
test-sc64-carrier-save-flags: build/test-sc64-carrier-save-flags-base build/test-sc64-carrier-save-flags-swarm
	./build/test-sc64-carrier-save-flags-base
	./build/test-sc64-carrier-save-flags-swarm
