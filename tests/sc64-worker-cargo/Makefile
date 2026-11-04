# Optional cargo checks use prepared inputs only; no asset generation recipes.
WORKER_CARGO_terran_FLAGS = $(TERRAN_ROM_FLAGS)
WORKER_CARGO_z6_FLAGS = $(ZERG_ROM_FLAGS)
WORKER_CARGO_p7_FLAGS = $(PROTOSS_P7_CANONICAL_FLAGS)
WORKER_CARGO_z7_FLAGS = $(ZERG_Z7_CANONICAL_FLAGS)
WORKER_CARGO_showcase_FLAGS = $(SHOWCASE_FLAGS)
WORKER_CARGO_terran_SOURCES = starcraft/sc64_bc_body_art.c
WORKER_CARGO_z6_SOURCES = starcraft/sc64_bc_body_art.c
WORKER_CARGO_p7_SOURCES = $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c
WORKER_CARGO_showcase_SOURCES = $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c

define SC64_WORKER_GATHER_FIXTURE
build/test-sc64-worker-gather-$(1): tests/sc64_worker_gather_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
build/test-sc64-worker-cargo-save-$(1): tests/sc64_worker_cargo_save_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
build/test-sc64-worker-cargo-ai-$(1): tests/sc64_worker_cargo_ai_game.c $(STARCRAFT) $$(wildcard starcraft/*.h) common/gba.c common/gba.h Makefile tests/sc64-worker-cargo/Makefile | build
	$$(CLANG) $$(P7_ARBITER_TEST_FLAGS) $$(WORKER_CARGO_$(1)_FLAGS) -DSC64_WORKER_CARGO_KIND $$< common/gba.c $$(WORKER_CARGO_$(1)_SOURCES) -o $$@
.PHONY: test-sc64-worker-gather-$(1)
test-sc64-worker-gather-$(1): build/test-sc64-worker-gather-$(1) build/test-sc64-worker-cargo-save-$(1) build/test-sc64-worker-cargo-ai-$(1)
	./build/test-sc64-worker-gather-$(1)
	./build/test-sc64-worker-cargo-save-$(1)
	./build/test-sc64-worker-cargo-ai-$(1)
endef
$(foreach profile,terran z6 p7 z7 showcase,$(eval $(call SC64_WORKER_GATHER_FIXTURE,$(profile))))

.PHONY: test-sc64-worker-gather
test-sc64-worker-gather: $(foreach profile,terran z7 p7 showcase,test-sc64-worker-gather-$(profile))
