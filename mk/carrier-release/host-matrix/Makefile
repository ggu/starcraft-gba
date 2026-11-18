# SPDX-License-Identifier: GPL-3.0-or-later
# Run with prepared local inputs and -j1. Keep journals and test binaries temporary.
SC64_CARRIER_MATRIX_HOST = $(CLANG) $(filter-out -g,$(SHARED_MECHANICS_HOST_FLAGS))
SC64_CARRIER_MATRIX_SOURCES = common/gba.c $(filter-out starcraft/game.c,$(STARCRAFT))
SC64_CARRIER_MATRIX_GUARD = $(PYTHON) -c 'import shutil, sys; sys.exit("Carrier HOST matrix needs 5 GiB free" if shutil.disk_usage("build").free < 5 * 2**30 else 0)'

define SC64_CARRIER_RACE_HOST_MATRIX
.PHONY: test-carrier-release-matrix-$(1)
test-carrier-release-matrix-$(1): tests/sc64_carrier_campaign_journal.c tests/sc64_campaign_progression.h tests/sc64_skirmish_opening_menu.c tests/sc64_skirmish_campaign_run.c $$($(3)) $$(SOURCE_WEAPON_INPUTS) | build
	@set -eu; \
	run=$$$$(mktemp -d build/carrier-matrix-$(1).XXXXXX); trap 'rm -rf "$$$$run"' EXIT; \
	$$(SC64_CARRIER_MATRIX_GUARD); \
	$$(SC64_CARRIER_MATRIX_HOST) $$($(2)) tests/sc64_carrier_campaign_journal.c $$(SC64_CARRIER_MATRIX_SOURCES) -o "$$$$run/test"; \
	"$$$$run/test" "$$$$run/normal"; \
	$$(SC64_CARRIER_MATRIX_GUARD); \
	$$(SC64_CARRIER_MATRIX_HOST) $$($(2)) $$(SC64_CARRIER_RELEASE_FLAGS) tests/sc64_carrier_campaign_journal.c $$(SC64_CARRIER_MATRIX_SOURCES) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o "$$$$run/test"; \
	"$$$$run/test" "$$$$run/selector"; \
	cmp "$$$$run/normal" "$$$$run/selector"; \
	$$(PYTHON) -c 'import hashlib, pathlib, sys; print("$(1) campaign payloads byte-identical:", hashlib.sha256(pathlib.Path(sys.argv[1]).read_bytes()).hexdigest())' "$$$$run/normal"; \
	$$(SC64_CARRIER_MATRIX_GUARD); \
	$$(SC64_CARRIER_MATRIX_HOST) $$($(2)) $$(SC64_CARRIER_RELEASE_FLAGS) tests/sc64_skirmish_opening_menu.c $$(SC64_CARRIER_MATRIX_SOURCES) $$(P7_INTERCEPTOR_FLIGHT_SOURCES) -o "$$$$run/test"; \
	"$$$$run/test"
endef
$(eval $(call SC64_CARRIER_RACE_HOST_MATRIX,terran,TERRAN_ROM_FLAGS,TERRAN_CARRIER_PROBE_DEPS))
$(eval $(call SC64_CARRIER_RACE_HOST_MATRIX,zerg-z10,ZERG_Z10_RELEASE_FLAGS,ZERG_CARRIER_PROBE_DEPS))

.PHONY: test-carrier-release-matrix-showcase test-carrier-release-matrix-protoss-p10
test-carrier-release-matrix-showcase: tests/sc64_showcase_game.c tests/sc64_skirmish_opening_menu.c tests/sc64_skirmish_campaign_run.c $(SHOWCASE_CARRIER_DEPS) $(SHOWCASE_CARRIER_FACT_HEADER) | build
	@set -eu; \
	run=$$(mktemp -d build/carrier-matrix-showcase.XXXXXX); trap 'rm -rf "$$run"' EXIT; \
	$(SC64_CARRIER_MATRIX_GUARD); \
	$(CLANG) $(filter-out -g,$(P7_ARBITER_TEST_FLAGS)) $(SHOWCASE_FLAGS) tests/sc64_showcase_game.c $(SC64_CARRIER_MATRIX_SOURCES) -o "$$run/test"; \
	"$$run/test"; \
	$(SC64_CARRIER_MATRIX_GUARD); \
	$(CLANG) $(filter-out -g,$(P7_ARBITER_TEST_FLAGS)) $(SHOWCASE_CARRIER_FLAGS) tests/sc64_showcase_game.c $(SC64_CARRIER_MATRIX_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o "$$run/test"; \
	"$$run/test"; \
	$(SC64_CARRIER_MATRIX_GUARD); \
	$(SC64_CARRIER_MATRIX_HOST) $(SHOWCASE_CARRIER_FLAGS) tests/sc64_skirmish_opening_menu.c $(SC64_CARRIER_MATRIX_SOURCES) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o "$$run/test"; \
	"$$run/test"

# Protoss already has the Carrier service and its selector flags are unchanged.
test-carrier-release-matrix-protoss-p10: test-sc64-p10-gantrithor
