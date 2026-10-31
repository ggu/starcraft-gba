# Asset-free effect lifetime stage; no gameplay capability is enabled.
.PHONY: test-dark-swarm-state
build/test-dark-swarm-state: tests/sc64_dark_swarm_state.c starcraft/sc64_dark_swarm_state.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
test-dark-swarm-state: build/test-dark-swarm-state
	./build/test-dark-swarm-state

# Experimental actual-actor check; the shared Defiler identity gate is test-only.
.PHONY: test-dark-swarm-cast-probe
build/test-dark-swarm-cast-probe: tests/sc64_dark_swarm_cast_game.c starcraft/sc64_dark_swarm_cast_probe.h starcraft/sc64_dark_swarm_state.h $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_DEFILER_STAGE -DSC64_DARK_SWARM_CAST_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-cast-probe: build/test-dark-swarm-cast-probe
	./build/test-dark-swarm-cast-probe

# Disabled shared Defiler executor; requires prepared Showcase inputs.
.PHONY: test-dark-swarm-executor
build/test-dark-swarm-executor: tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-executor: build/test-dark-swarm-executor
	./build/test-dark-swarm-executor

# Requires privately generated, authenticated image 337 and prepared inputs.
DARK_SWARM_ART_DIR ?= .local/dark-swarm-art-v1
.PHONY: test-dark-swarm-render
build/test-dark-swarm-render: tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(DARK_SWARM_ART_DIR)/dark_swarm_source_generated.h $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_ART_PROBE -I$(DARK_SWARM_ART_DIR) $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-render: build/test-dark-swarm-render
	./build/test-dark-swarm-render

# Requires retained pinned OpenBW flat-world oracle logs and prepared inputs.
DARK_SWARM_ORACLE_DIR ?= .local/defiler-approach-audit
.PHONY: test-dark-swarm-approach
build/test-dark-swarm-approach: tests/sc64_defiler_approach_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_APPROACH_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-approach: build/test-dark-swarm-approach
	./build/test-dark-swarm-approach > build/test-dark-swarm-approach.log
	$(PYTHON) tests/sc64_compare_defiler_approach.py --actual build/test-dark-swarm-approach.log --oracle-dir $(DARK_SWARM_ORACLE_DIR)

# Disabled raw-52 identity and natural-energy actual-game probe.
.PHONY: test-dark-swarm-hero
build/test-dark-swarm-hero: tests/sc64_defiler_hero_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_HERO_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-hero: build/test-dark-swarm-hero
	./build/test-dark-swarm-hero

.PHONY: test-dark-swarm-approach-ui
build/test-dark-swarm-approach-ui: tests/sc64_defiler_approach_ui_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_APPROACH_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-approach-ui: build/test-dark-swarm-approach-ui
	./build/test-dark-swarm-approach-ui

# Disabled field/weapon-launch query; uses prepared Showcase and source facts.
.PHONY: test-dark-swarm-query
build/test-dark-swarm-query: tests/sc64_dark_swarm_query_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_HERO_PROBE -DSC64_SHARED_DARK_SWARM_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-query: build/test-dark-swarm-query
	./build/test-dark-swarm-query

# Synthetic authored unit through the actual campaign import path.
.PHONY: test-dark-swarm-hero-import
build/test-dark-swarm-hero-import: tests/sc64_defiler_import_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_HERO_PROBE -DSC64_DEFILER_IMPORT_FIXTURE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-hero-import: build/test-dark-swarm-hero-import
	./build/test-dark-swarm-hero-import

# Source-rectangle table is generated privately from authenticated units.dat.
DARK_SWARM_RECTS_DIR ?= .local/dark-swarm-rects-v1
DARK_SWARM_PROFILE_FLAGS ?= $(SHOWCASE_FLAGS)
.PHONY: test-dark-swarm-rect-lookup
build/test-dark-swarm-rect-lookup: tests/sc64_dark_swarm_rect_lookup.c starcraft/sc64_dark_swarm_rect_lookup.h $(DARK_SWARM_RECTS_DIR)/dark_swarm_rects_generated.h Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -Istarcraft -I$(DARK_SWARM_RECTS_DIR) $< -o $@
test-dark-swarm-rect-lookup: build/test-dark-swarm-rect-lookup
	./build/test-dark-swarm-rect-lookup

.PHONY: test-dark-swarm-source-rects
build/test-dark-swarm-source-rects: tests/sc64_dark_swarm_query_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(DARK_SWARM_RECTS_DIR)/dark_swarm_rects_generated.h $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_HERO_PROBE -DSC64_SHARED_DARK_SWARM_PROBE -DSC64_SHARED_DARK_SWARM_SOURCE_RECTS -I$(DARK_SWARM_RECTS_DIR) $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-source-rects: build/test-dark-swarm-source-rects
	./build/test-dark-swarm-source-rects

.PHONY: test-dark-swarm-profile
build/test-dark-swarm-profile: tests/sc64_dark_swarm_profile_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(DARK_SWARM_RECTS_DIR)/dark_swarm_rects_generated.h common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DARK_SWARM_PROBE -DSC64_SHARED_DARK_SWARM_SOURCE_RECTS -I$(DARK_SWARM_RECTS_DIR) $(DARK_SWARM_PROFILE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-profile: build/test-dark-swarm-profile
	./build/test-dark-swarm-profile

# Natural unmodified Defiler energy regeneration through the actual update loop.
.PHONY: test-dark-swarm-natural
build/test-dark-swarm-natural: tests/sc64_defiler_natural_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-natural: build/test-dark-swarm-natural
	./build/test-dark-swarm-natural

# Restricted southeast clear-approach oracle; source logs are private inputs.
DARK_SWARM_SE_ORACLE_DIR ?= .local/defiler-se-lengths
.PHONY: test-dark-swarm-se-approach test-dark-swarm-se-guard
build/test-dark-swarm-se-approach: tests/sc64_defiler_se_distance_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_APPROACH_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-se-approach: build/test-dark-swarm-se-approach
	$(PYTHON) tests/sc64_verify_defiler_se_set.py --game $< --source-dir $(DARK_SWARM_SE_ORACLE_DIR) --out build/defiler-se-verification
build/test-dark-swarm-se-guard: tests/sc64_defiler_se_guard_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_APPROACH_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-se-guard: build/test-dark-swarm-se-guard
	./build/test-dark-swarm-se-guard
build/test-dark-swarm-se-hero-guard: tests/sc64_defiler_se_guard_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_HERO_PROBE -DSC64_SHARED_DEFILER_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_APPROACH_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
.PHONY: test-dark-swarm-se-hero-guard
test-dark-swarm-se-hero-guard: build/test-dark-swarm-se-hero-guard
	./build/test-dark-swarm-se-hero-guard

# Independently gated raw-52 source trajectories; retained source logs are inputs.
DARK_SWARM_SE_HERO_ORACLE_DIR ?= .local/defiler-se-hero-set
.PHONY: test-dark-swarm-se-hero-approach test-dark-swarm-se-hero-enabled-guard
build/test-dark-swarm-se-hero-approach: tests/sc64_defiler_se_distance_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_HERO_PROBE -DSC64_SHARED_DEFILER_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_HERO_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-se-hero-approach: build/test-dark-swarm-se-hero-approach
	$(PYTHON) tests/sc64_verify_defiler_se_set.py --game $< --source-dir $(DARK_SWARM_SE_HERO_ORACLE_DIR) --out build/defiler-se-hero-verification
build/test-dark-swarm-se-hero-enabled-guard: tests/sc64_defiler_se_guard_game.c tests/sc64_defiler_executor_game.c $(STARCRAFT) $(wildcard starcraft/*.h) $(SHOWCASE_INPUTS) common/gba.c Makefile | build
	$(CLANG) $(SHARED_MECHANICS_HOST_FLAGS) -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_ORDINARY_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_HERO_PROBE -DSC64_SHARED_DEFILER_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_HERO_PROBE $(SHOWCASE_FLAGS) $< common/gba.c $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
test-dark-swarm-se-hero-enabled-guard: build/test-dark-swarm-se-hero-enabled-guard
	./build/test-dark-swarm-se-hero-enabled-guard

# Noncompact malformed-coordinate guard; uses the existing prepared core inputs.
.PHONY: test-dark-swarm-noncompact-guard
build/test-dark-swarm-noncompact-guard: tests/sc64_defiler_noncompact_guard_game.c $(STARCRAFT) $(wildcard starcraft/*.h) common/gba.c Makefile | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -fsanitize=address,undefined -DGBA_HOST -Icommon -Istarcraft -I.local/sc64/races/generated -I.local/sc64/backwater/generated -I.local/sc64/interface/generated -I.local/sc64/briefings/generated -I.local/sc64/skirmish/generated -I.local/sc64/bc/generated -DSC64_IMPORTED_MISSION -DSC64_UNIFIED -DSC64_IMPORTED_SKIRMISH -DSC64_RACES -DSC64_SHARED_PROJECTILE_STAGE -DSC64_SHARED_DEFILER_STAGE -DSC64_SHARED_DEFILER_EXECUTOR -DSC64_SHARED_DEFILER_APPROACH_PROBE -DSC64_SHARED_DEFILER_SE_APPROACH_PROBE -DSC64_BATTLECRUISER_EXPERIMENT -DSC64_BATTLECRUISER_ENERGY_EXPERIMENT $< common/gba.c -o $@
test-dark-swarm-noncompact-guard: build/test-dark-swarm-noncompact-guard
	./build/test-dark-swarm-noncompact-guard

