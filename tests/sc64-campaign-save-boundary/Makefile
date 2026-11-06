# SPDX-License-Identifier: GPL-3.0-or-later
# Include top-level and ARM qualification Makefiles first. Read-only HOST probe.
CLANG ?= clang
BOUNDARY_DIAG_ROOT ?= .local/campaign-save-boundary/traced
BOUNDARY_OUT ?= .local/campaign-save-boundary
BOUNDARY_HOST_FLAGS = -std=c11 -O1 -g0 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable -Wno-misleading-indentation -fsanitize=address,undefined -DGBA_HOST -Icommon
.PHONY: campaign-save-boundary
campaign-save-boundary:
	test -f "$(BOUNDARY_SEED)"
	mkdir -p $(BOUNDARY_OUT)
	python3 tests/sc64-campaign-save-boundary/instrument.py $(BOUNDARY_DIAG_ROOT)
	$(CLANG) $(BOUNDARY_HOST_FLAGS) $(filter-out -DSC64_SHARED_ACTOR_ARM_QUALIFICATION,$(ARMQ_STAGE)) $(ARMQ_PROFILE) $(BC_FLAGS) $(ZERG_Z7_RELEASE_FLAGS) -DSC64_Z7_DERIVED_SAVE '-DSC64_BOUNDARY_GAME="$(abspath $(BOUNDARY_DIAG_ROOT))/starcraft/game.c"' tests/sc64-campaign-save-boundary/probe.c tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(BOUNDARY_OUT)/probe
	$(BOUNDARY_OUT)/probe "$(BOUNDARY_SEED)"
.PHONY: campaign-envelope-metadata
campaign-envelope-metadata:
	mkdir -p $(BOUNDARY_OUT)
	$(CLANG) -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined tests/sc64-campaign-save-boundary/envelope.c -o $(BOUNDARY_OUT)/envelope
	$(BOUNDARY_OUT)/envelope "$(BOUNDARY_SEED)"
.PHONY: campaign-legacy-rows
campaign-legacy-rows:
	test -f "$(BOUNDARY_SEED)"
	mkdir -p $(BOUNDARY_OUT)
	$(CLANG) $(BOUNDARY_HOST_FLAGS) $(filter-out -DSC64_SHARED_ACTOR_ARM_QUALIFICATION,$(ARMQ_STAGE)) $(ARMQ_PROFILE) $(BC_FLAGS) $(ZERG_Z7_RELEASE_FLAGS) -DSC64_Z7_DERIVED_SAVE -DSC64_Z7_LEGACY_ROWS_HOST_PROBE tests/sc64-campaign-save-boundary/rows.c tests/sc64_guardian_journal_shim.c $(filter-out starcraft/game.c,$(STARCRAFT)) $(P7_INTERCEPTOR_FLIGHT_SOURCES) -o $(BOUNDARY_OUT)/rows
	$(BOUNDARY_OUT)/rows "$(BOUNDARY_SEED)"
