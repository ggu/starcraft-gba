# Optional cargo checks use prepared inputs only; no asset generation recipes.
# Supply an additional verified include directory when required by a prepared bank.
WORKER_CARGO_CPPFLAGS ?=
WORKER_CARGO_LEGACY_SOURCE ?= tests/sc64_worker_gather_legacy_fresh.c
WORKER_CARGO_terran_FLAGS = $(TERRAN_ROM_FLAGS)
WORKER_CARGO_z6_FLAGS = $(ZERG_ROM_FLAGS)
WORKER_CARGO_p7_FLAGS = $(PROTOSS_P7_CANONICAL_FLAGS)
WORKER_CARGO_p10_FLAGS = $(PROTOSS_P10_FLAGS)
WORKER_CARGO_z7_FLAGS = $(ZERG_Z7_CANONICAL_FLAGS)
WORKER_CARGO_showcase_FLAGS = $(SHOWCASE_FLAGS)
WORKER_CARGO_terran_SOURCES = starcraft/sc64_bc_body_art.c
WORKER_CARGO_z6_SOURCES = starcraft/sc64_bc_body_art.c
WORKER_CARGO_p7_SOURCES = $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c
WORKER_CARGO_p10_SOURCES = $(WORKER_CARGO_p7_SOURCES)
WORKER_CARGO_showcase_SOURCES = $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c

define SC64_WORKER_GATHER_FIXTURE
build/test-sc64-worker-gather-$(1): tests/sc64_worker_gather_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_CPPFLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
build/test-sc64-worker-depletion-$(1): tests/sc64_worker_mineral_depletion_game.c tests/sc64_worker_gather_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_CPPFLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
build/test-sc64-worker-cycle-$(1): tests/sc64_worker_collection_cycle_test.c tests/sc64_worker_gather_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_CPPFLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
build/test-sc64-worker-cycle-save-$(1): tests/sc64_worker_gather_save_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_CPPFLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
build/test-sc64-worker-cargo-save-$(1): tests/sc64_worker_cargo_save_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_CPPFLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
build/test-sc64-worker-cargo-ai-$(1): tests/sc64_worker_cargo_ai_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_CPPFLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
.PHONY: test-sc64-worker-gather-$(1)
test-sc64-worker-gather-$(1): build/test-sc64-worker-cycle-$(1) build/test-sc64-worker-cycle-save-$(1) build/test-sc64-worker-depletion-$(1) build/test-sc64-worker-gather-$(1) build/test-sc64-worker-cargo-save-$(1) build/test-sc64-worker-cargo-ai-$(1)
	./build/test-sc64-worker-cycle-$(1)
	./build/test-sc64-worker-cycle-save-$(1)
	./build/test-sc64-worker-depletion-$(1)
	./build/test-sc64-worker-gather-$(1)
	./build/test-sc64-worker-cargo-save-$(1)
	./build/test-sc64-worker-cargo-ai-$(1)
endef
$(foreach profile,terran z6 p7 p10 z7 showcase,$(eval $(call SC64_WORKER_GATHER_FIXTURE,$(profile))))

.PHONY: test-sc64-worker-gather
test-sc64-worker-gather: $(foreach profile,terran z7 p7 p10 showcase,test-sc64-worker-gather-$(profile))

# Separate P10 movement check: prepared P10 inputs required. The authored scene
# is initialized once; real worker updates perform the post-retarget trip.
build/test-sc64-worker-route-p10: tests/sc64_worker_depletion_route_review.c tests/sc64_worker_gather_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(WORKER_CARGO_p10_FLAGS) -DSC64_WORKER_CARGO_KIND $< common/gba.c $(WORKER_CARGO_p10_SOURCES) -o $@
.PHONY: test-sc64-worker-route-p10
test-sc64-worker-route-p10: build/test-sc64-worker-route-p10
	./build/test-sc64-worker-route-p10

# Optional cross-revision producer/consumer: build separately in each tree.
build/test-sc64-worker-legacy-p10: $(WORKER_CARGO_LEGACY_SOURCE) tests/sc64_pre_cargo_compat_shim.c tests/sc64_guardian_journal_shim.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(WORKER_CARGO_CPPFLAGS) $(WORKER_CARGO_p10_FLAGS) -DSC64_WORKER_CARGO_KIND $(WORKER_CARGO_LEGACY_SOURCE) tests/sc64_pre_cargo_compat_shim.c $(WORKER_CARGO_p10_SOURCES) -o $@
