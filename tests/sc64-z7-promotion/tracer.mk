# Z7 uses the uncached lifetime path; test its production behavior directly.
build/test-sc64-tracer-profile-zerg: tests/sc64_z7_tracer_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(CANONICAL_ZERG_PROFILE_INPUTS) $(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile | build
	$(CLANG) $(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $(CANONICAL_ZERG_PROFILE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@

define CANONICAL_TRACER_PROFILE_FIXTURE
build/test-sc64-tracer-profile-$(1): tests/sc64_trace_profile_cache_game.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$($(3)) $$($(4)) $$(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $$($(2)) $$< common/gba.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$($(4)) -o $$@
endef
$(eval $(call CANONICAL_TRACER_PROFILE_FIXTURE,terran,TERRAN_ROM_FLAGS,TERRAN_FULL_INPUTS))
# The Z7 release does not enable tracer-duration caching; keep the cache
# fixture explicitly tied to the older Z6 vector.
$(eval $(call CANONICAL_TRACER_PROFILE_FIXTURE,zerg-z6,ZERG_ROM_FLAGS,ZERG_Z6_INPUTS))
$(eval $(call CANONICAL_TRACER_PROFILE_FIXTURE,protoss,PROTOSS_ROM_FLAGS,PROTOSS_CANONICAL_INPUTS,P7_INTERCEPTOR_FLIGHT_SOURCES))
$(eval $(call CANONICAL_TRACER_PROFILE_FIXTURE,showcase,SHOWCASE_FLAGS,SHOWCASE_INPUTS))
.PHONY: test-sc64-canonical-tracer-profile
test-sc64-canonical-tracer-profile: $(addprefix build/test-sc64-tracer-profile-,terran zerg protoss showcase)
	./build/test-sc64-tracer-profile-terran
	./build/test-sc64-tracer-profile-zerg
	./build/test-sc64-tracer-profile-protoss
	./build/test-sc64-tracer-profile-showcase
.PHONY: test-sc64-zerg-z6-tracer-profile
test-sc64-zerg-z6-tracer-profile: build/test-sc64-tracer-profile-zerg-z6
	./build/test-sc64-tracer-profile-zerg-z6


define CANONICAL_SKIRMISH_RESUME_PROFILE
build/test-sc64-skirmish-resume-$(1): tests/sc64_skirmish_resume.c $$(STARCRAFT) $$(wildcard starcraft/*.h) $$($(3)) $$($(4)) $$(CANONICAL_SKIRMISH_RESUME_ART_INPUTS) common/gba.c common/gba.h Makefile | build
	$$(CLANG) $$(CANONICAL_SKIRMISH_RESUME_HOST_FLAGS) $$($(2)) $$< common/gba.c $$(filter-out starcraft/game.c,$$(STARCRAFT)) $$($(4)) -o $$@
endef
.PHONY: test-sc64-canonical-skirmish-resume
test-sc64-canonical-skirmish-resume: $(addprefix build/test-sc64-skirmish-resume-,terran zerg protoss)
	./build/test-sc64-skirmish-resume-terran
	./build/test-sc64-skirmish-resume-zerg
	./build/test-sc64-skirmish-resume-protoss
test-race-campaigns: test-sc64-canonical-skirmish-resume
