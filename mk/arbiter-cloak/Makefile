# SPDX-License-Identifier: GPL-3.0-or-later
# Included from the top-level Makefile after the P10 rules; it is not a
# standalone makefile. Run these targets from the repository root.
#
# Differential test of the Arbiter cloak and detection bookkeeping against a
# frozen copy of the pre-change code (tests/sc64_arbiter_cloak_oracle.h). The
# same test is built for the shipping Protoss P10 vector, the canonical P7
# vector, the plain P7 Arbiter vectors without and with the rectangle hoist,
# and Showcase, all with ASan and UBSan.
#   make -o <p4 scenario header> test-sc64-arbiter-cloak [ARBITER_CLOAK_ARGS="worlds updates seed"]
ARBITER_CLOAK_ARGS ?=
ARBITER_CLOAK_DEPS = tests/sc64_arbiter_cloak_diff.c tests/sc64_arbiter_cloak_oracle.h $(STARCRAFT) $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) $(wildcard starcraft/*.h) common/gba.c $(COMMON_HEADERS) Makefile mk/arbiter-cloak/Makefile | build

build/test-sc64-arbiter-cloak-p10: $(ARBITER_CLOAK_DEPS) $(PROTOSS_P10_INPUTS)
	$(CLANG) $(P8_HOST_FLAGS) $(PROTOSS_P10_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-arbiter-cloak-p7: $(ARBITER_CLOAK_DEPS) $(PROTOSS_P7_CANONICAL_INPUTS)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(PROTOSS_P7_CANONICAL_FLAGS) $< common/gba.c $(P7_PROJECTILE_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) starcraft/sc64_bc_body_art.c -o $@
build/test-sc64-arbiter-cloak-plain: $(ARBITER_CLOAK_DEPS) $(P7_ARBITER_INPUTS)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_ARBITER_FLAGS) -DSC64_P7_PREVALIDATED_TICK -DSC64_P7_COUNT_SNAPSHOT $< common/gba.c -o $@
build/test-sc64-arbiter-cloak-rect: $(ARBITER_CLOAK_DEPS) $(P7_ARBITER_INPUTS)
	$(CLANG) $(P7_ARBITER_TEST_FLAGS) $(P7_TYPED_RECT_FLAGS) $< common/gba.c -o $@
build/test-sc64-arbiter-cloak-showcase: $(ARBITER_CLOAK_DEPS) $(P7_ARBITER_INPUTS) $(UNIT_FACING_INPUTS) $(COMMAND_ICON_HEADER) $(TANK_SOURCE_HEADER) $(CANONICAL_DEATH_INPUTS) $(SOURCE_WEAPON_INPUTS)
	$(CLANG) $(SKIRMISH_RUNTIME_TEST_FLAGS) $(SHOWCASE_FLAGS) $< common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT)) -o $@

ARBITER_CLOAK_TESTS := build/test-sc64-arbiter-cloak-p10 build/test-sc64-arbiter-cloak-p7 build/test-sc64-arbiter-cloak-plain build/test-sc64-arbiter-cloak-rect build/test-sc64-arbiter-cloak-showcase
.PHONY: test-sc64-arbiter-cloak
test-sc64-arbiter-cloak: $(ARBITER_CLOAK_TESTS)
	for t in $(ARBITER_CLOAK_TESTS); do ./$$t $(ARBITER_CLOAK_ARGS) || exit 1; done
