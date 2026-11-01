# Explicit host-only qualification; no product gate or save namespace enabled.
HP_REGEN_HOST_FLAGS := $(P7_ARBITER_TEST_FLAGS) -DSC64_SHARED_HP_REGEN -DSC64_SHARED_HP_REGEN_TEST_ONLY
HP_REGEN_FIXTURE := tests/sc64_shared_hp_regen_game.c
HP_REGEN_COMMON := common/gba.c
HP_REGEN_P7_COMMON := $(HP_REGEN_COMMON) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c

.PHONY: test-sc64-shared-hp-regen-prepared
build/test-sc64-hp-regen-helper: tests/sc64_shared_hp_regen.c starcraft/sc64_shared_hp_regen.h | build
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined $< -o $@
build/test-sc64-hp-regen-z5: $(HP_REGEN_FIXTURE) $(STARCRAFT) $(Z5_INPUTS) common/gba.c | build
	$(CLANG) $(HP_REGEN_HOST_FLAGS) $(Z5_FLAGS) $< $(HP_REGEN_COMMON) -o $@
build/test-sc64-hp-regen-p6: $(HP_REGEN_FIXTURE) $(STARCRAFT) $(P6_INPUTS) common/gba.c | build
	$(CLANG) $(HP_REGEN_HOST_FLAGS) $(PROTOSS_P6_CANONICAL_FLAGS) $< $(HP_REGEN_COMMON) -o $@
build/test-sc64-hp-regen-z7: $(HP_REGEN_FIXTURE) $(STARCRAFT) $(ZERG_Z7_INPUTS) common/gba.c | build
	$(CLANG) $(HP_REGEN_HOST_FLAGS) $(ZERG_Z7_CANONICAL_FLAGS) $< $(HP_REGEN_COMMON) -o $@
build/test-sc64-hp-regen-p7: $(HP_REGEN_FIXTURE) $(STARCRAFT) $(PROTOSS_P7_CANONICAL_INPUTS) $(HP_REGEN_P7_COMMON) | build
	$(CLANG) $(HP_REGEN_HOST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< $(HP_REGEN_P7_COMMON) -o $@
build/test-sc64-hp-regen-showcase: $(HP_REGEN_FIXTURE) $(STARCRAFT) $(SHOWCASE_INPUTS) common/gba.c | build
	$(CLANG) $(HP_REGEN_HOST_FLAGS) $(SHOWCASE_FLAGS) $< $(HP_REGEN_COMMON) starcraft/sc64_bc_body_art.c -o $@
test-sc64-shared-hp-regen-prepared: build/test-sc64-hp-regen-helper build/test-sc64-hp-regen-z5 build/test-sc64-hp-regen-p6 build/test-sc64-hp-regen-z7 build/test-sc64-hp-regen-p7 build/test-sc64-hp-regen-showcase
	./build/test-sc64-hp-regen-helper
	./build/test-sc64-hp-regen-z5
	./build/test-sc64-hp-regen-p6
	./build/test-sc64-hp-regen-z7
	./build/test-sc64-hp-regen-p7
	./build/test-sc64-hp-regen-showcase
