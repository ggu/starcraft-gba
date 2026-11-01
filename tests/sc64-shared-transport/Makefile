# Asset-free bounded transport research and source-fact checks.
build/test-sc64-shared-transport-tech: tests/sc64_skirmish_transport_tech.c starcraft/sc64_skirmish_transport_tech.h starcraft/sc64_transport_rules.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
.PHONY: test-sc64-shared-transport-tech test-sc64-shared-transport-profiles
test-sc64-shared-transport-tech: build/test-sc64-shared-transport-tech
	./build/test-sc64-shared-transport-tech
test: test-sc64-shared-transport-tech

# Custom-game checks use activated product flags and prepared private banks.
# Legacy Z6 is an explicit feature-on comparison only.
define SC64_SHARED_TRANSPORT_PROFILE
build/test-sc64-shared-transport-$(1): tests/sc64_shared_transport_game.c tests/sc64_skirmish_resume.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$($(3)) $$($(4)) $$(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $$($(2)) $(5) $$< common/gba.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$($(4)) -o $$@
endef
$(eval $(call SC64_SHARED_TRANSPORT_PROFILE,terran,TERRAN_ROM_FLAGS,TERRAN_FULL_INPUTS))
$(eval $(call SC64_SHARED_TRANSPORT_PROFILE,zerg-z6,ZERG_ROM_FLAGS,ZERG_Z6_INPUTS,,-DSC64_SHARED_TRANSPORT))
$(eval $(call SC64_SHARED_TRANSPORT_PROFILE,zerg-z7,ZERG_Z7_CANONICAL_FLAGS,ZERG_Z7_CANONICAL_INPUTS))
# Z8 uses its own private bank and is qualified separately from the normal matrix.
$(eval $(call SC64_SHARED_TRANSPORT_PROFILE,zerg-z8,ZERG_Z8_CANONICAL_FLAGS,ZERG_Z8_CANONICAL_INPUTS))
$(eval $(call SC64_SHARED_TRANSPORT_PROFILE,protoss,PROTOSS_ROM_FLAGS,PROTOSS_CANONICAL_INPUTS,P7_INTERCEPTOR_FLIGHT_SOURCES))
$(eval $(call SC64_SHARED_TRANSPORT_PROFILE,p8,PROTOSS_P8_FLAGS,PROTOSS_P8_INPUTS,P7_INTERCEPTOR_FLIGHT_SOURCES))
$(eval $(call SC64_SHARED_TRANSPORT_PROFILE,showcase,SHOWCASE_FLAGS,SHOWCASE_INPUTS,P7_INTERCEPTOR_FLIGHT_SOURCES))
test-sc64-shared-transport-profiles: $(addprefix build/test-sc64-shared-transport-,terran zerg-z6 zerg-z7 protoss p8 showcase)
	@set -e; for profile in terran zerg-z6 zerg-z7 protoss p8 showcase; do ./build/test-sc64-shared-transport-$$profile; done


# Existing custom-game Dropships use the same pending-order lifecycle even
# when the staged shared-transport capability is disabled.
define SC64_PENDING_TRANSPORT_LEGACY_PROFILE
build/test-sc64-pending-transport-legacy-$(1): tests/sc64_transport_pending_legacy_game.c tests/sc64_skirmish_resume.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$($(3)) $$(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $$(filter-out -DSC64_SHARED_TRANSPORT,$$($(2))) $$< common/gba.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) -o $$@
endef
$(eval $(call SC64_PENDING_TRANSPORT_LEGACY_PROFILE,terran,TERRAN_ROM_FLAGS,TERRAN_FULL_INPUTS))
$(eval $(call SC64_PENDING_TRANSPORT_LEGACY_PROFILE,zerg-z6,ZERG_ROM_FLAGS,ZERG_Z6_INPUTS))
$(eval $(call SC64_PENDING_TRANSPORT_LEGACY_PROFILE,zerg-z7,ZERG_Z7_CANONICAL_FLAGS,ZERG_Z7_CANONICAL_INPUTS))
.PHONY: test-sc64-pending-transport-legacy
test-sc64-pending-transport-legacy: $(addprefix build/test-sc64-pending-transport-legacy-,terran zerg-z6 zerg-z7)
	@set -e; for profile in terran zerg-z6 zerg-z7; do ./build/test-sc64-pending-transport-legacy-$$profile; done

# Mission-specific source actors and permissions; fixtures write only normal
# saves and do not run the negative-SRAM cases in the parent test sources.
build/test-sc64-pending-transport-p5: tests/sc64_transport_pending_p5.c tests/sc64_p5_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(P5_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(P5_SHUTTLE_FLAGS) $< common/gba.c -o $@
build/test-sc64-pending-transport-z6: tests/sc64_transport_pending_z6.c tests/sc64_z6_runtime.c $(STARCRAFT) $(wildcard starcraft/*.h) $(ZERG_Z6_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(ZERG_ROM_FLAGS) $< common/gba.c starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-sc64-pending-transport-missions
test-sc64-pending-transport-missions: build/test-sc64-pending-transport-p5 build/test-sc64-pending-transport-z6
	./build/test-sc64-pending-transport-p5
	./build/test-sc64-pending-transport-z6

build/test-sc64-campaign-shuttle-p7: tests/sc64_campaign_shared_shuttle.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_CANONICAL_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@
build/test-sc64-campaign-shuttle-p8: tests/sc64_campaign_shared_shuttle.c $(STARCRAFT) $(wildcard starcraft/*.h) $(PROTOSS_P8_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(PROTOSS_P8_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $@

build/test-sc64-t5-shared-transport-positive: tests/sc64_t5_shared_transport_positive.c tests/sc64_transport_engine.c $(STARCRAFT) $(wildcard starcraft/*.h) $(RACE_CAMPAIGN_INPUTS) $(T4_SCENARIO_HEADER) $(T5_SCENARIO_HEADER) $(T5_AVAILABILITY_HEADER) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(T5_PROBE_FLAGS) $(SC64_RELEASE_TRANSPORT_FLAGS) $< common/gba.c -o $@
